#include "cover_manager.h"
#include "../cache_manager.h"
#include "../file_utils.h"
#include "../image_loader.h"
#include <unordered_set>
#include <cstring>
#include <functional>

#ifdef __vita__
#include <psp2/io/dirent.h>
#endif

CoverManager& CoverManager::getInstance() {
    static CoverManager instance;
    return instance;
}

CoverManager::CoverManager() {
}

CoverManager::~CoverManager() {
    clearAllTextures();
}

void CoverManager::init() {
    CacheManager::getInstance().init();
    refreshDiskIndex();
}

void CoverManager::refreshDiskIndex() {
    m_diskCoverFiles.clear();
    std::string coverDir = CacheManager::getInstance().getCoverCachePath();

#ifdef __vita__
    SceUID dfd = sceIoDopen(coverDir.c_str());
    if (dfd >= 0) {
        SceIoDirent dir;
        while (sceIoDread(dfd, &dir) > 0) {
            if (SCE_S_ISDIR(dir.d_stat.st_mode)) continue;
            if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0) continue;
            m_diskCoverFiles.insert(dir.d_name);
        }
        sceIoDclose(dfd);
    }
#endif
    m_indexInitialized = true;
}

std::string CoverManager::getCoverPath(const std::string& bookPath) {
    return CacheManager::getInstance().getCoverFilePath(bookPath);
}

std::string CoverManager::getCoverFileName(const std::string& bookPath) {
    std::string fileName = FileUtils::getFileName(bookPath);
    std::hash<std::string> hasher;
    size_t hashValue = hasher(bookPath);
    return std::to_string(hashValue) + "_" + fileName + ".jpg";
}

bool CoverManager::hasCoverOnDisk(const std::string& bookPath) {
    if (bookPath.empty()) return false;
    if (!m_indexInitialized) {
        refreshDiskIndex();
    }

    std::string fileNameJpg = getCoverFileName(bookPath);
    if (m_diskCoverFiles.find(fileNameJpg) != m_diskCoverFiles.end()) {
        return true;
    }

    // Checagem de retrocompatibilidade com .png
    size_t dotPos = fileNameJpg.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string fileNamePng = fileNameJpg.substr(0, dotPos) + ".png";
        if (m_diskCoverFiles.find(fileNamePng) != m_diskCoverFiles.end()) {
            return true;
        }
    }

    return false;
}

size_t CoverManager::getCoverCountOnDisk() const {
    return m_diskCoverFiles.size();
}

vita2d_texture* CoverManager::getCoverTexture(const std::string& bookPath) {
    if (bookPath.empty()) return nullptr;

    auto it = m_textures.find(bookPath);
    if (it != m_textures.end()) {
        if (it->second != nullptr) {
            auto lruIt = m_lruMap.find(bookPath);
            if (lruIt != m_lruMap.end()) {
                m_lruList.erase(lruIt->second);
                m_lruList.push_front(bookPath);
                m_lruMap[bookPath] = m_lruList.begin();
            }
        }
        return it->second;
    }

    // Se não existir no índice de disco em RAM, não tenta abrir via I/O do kernel (zero erros de missing file)
    if (!hasCoverOnDisk(bookPath)) {
        m_textures[bookPath] = nullptr;
        return nullptr;
    }

    std::string jpgName = getCoverFileName(bookPath);
    std::string coverDir = CacheManager::getInstance().getCoverCachePath();
    std::string actualPath = "";

    if (m_diskCoverFiles.find(jpgName) != m_diskCoverFiles.end()) {
        actualPath = coverDir + "/" + jpgName;
    } else {
        size_t dotPos = jpgName.find_last_of('.');
        if (dotPos != std::string::npos) {
            std::string pngName = jpgName.substr(0, dotPos) + ".png";
            if (m_diskCoverFiles.find(pngName) != m_diskCoverFiles.end()) {
                actualPath = coverDir + "/" + pngName;
            }
        }
    }

    if (actualPath.empty()) {
        m_textures[bookPath] = nullptr;
        return nullptr;
    }

    vita2d_texture* tex = nullptr;
    std::string ext = FileUtils::getExtension(actualPath);
    if (ext == "jpg" || ext == "jpeg") {
        tex = vita2d_load_JPEG_file(actualPath.c_str());
    } else if (ext == "png") {
        tex = vita2d_load_PNG_file(actualPath.c_str());
    } else {
        tex = ImageLoader::loadTextureFromFile(actualPath);
    }

    m_textures[bookPath] = tex;
    if (tex != nullptr) {
        m_lruList.push_front(bookPath);
        m_lruMap[bookPath] = m_lruList.begin();
    }
    return tex;
}

void CoverManager::trimCache(size_t maxCount) {
    if (m_lruList.size() <= maxCount) return;

    vita2d_wait_rendering_done();
    while (m_lruList.size() > maxCount) {
        std::string oldest = m_lruList.back();
        m_lruList.pop_back();
        m_lruMap.erase(oldest);

        auto it = m_textures.find(oldest);
        if (it != m_textures.end()) {
            if (it->second) {
                vita2d_free_texture(it->second);
            }
            m_textures.erase(it);
        }
    }
}

void CoverManager::purgeUnused(const std::vector<std::string>& visibleBookPaths) {
    // Mantido como no-op para segurança da pipeline gráfica GXM.
    // O gerenciamento de memória é realizado por trimCache() fora do ciclo de desenho.
    (void)visibleBookPaths;
}

void CoverManager::clearAllTextures() {
    if (m_textures.empty() && m_lruList.empty()) return;

    vita2d_wait_rendering_done();
    for (auto& pair : m_textures) {
        if (pair.second) {
            vita2d_free_texture(pair.second);
        }
    }
    m_textures.clear();
    m_lruList.clear();
    m_lruMap.clear();
}

bool CoverManager::generateCover(const std::string& bookPath, bool force) {
    if (bookPath.empty()) return false;
    if (!force && hasCoverOnDisk(bookPath)) return true;

    std::string outPath = getCoverPath(bookPath);
    bool ok = ImageLoader::generateCoverThumbnail(bookPath, outPath, 120, 170);
    if (ok) {
        m_diskCoverFiles.insert(getCoverFileName(bookPath));
    }
    return ok;
}

int CoverManager::clearCoverCacheOnDisk() {
    clearAllTextures();
    int count = static_cast<int>(m_diskCoverFiles.size());
    CacheManager::getInstance().cleanupAllCovers();
    m_diskCoverFiles.clear();
    m_textures.clear();
    m_lruList.clear();
    m_lruMap.clear();
    return count;
}
