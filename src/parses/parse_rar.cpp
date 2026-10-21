#include "parse_rar.h"
#include "../file_utils.h"
#include "../ui_components.h"
#include "../image_loader.h"

#define _UNIX
#include "../../libs/unrar/dll.hpp"

#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <psp2/io/stat.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>

static int CALLBACK UnrarCallback(UINT msg, LPARAM UserData, LPARAM P1, LPARAM P2) {
    if (msg == UCM_PROCESSDATA) {
        // P1 = Address of data, P2 = Size of data
    }
    return 1;
}

ParseRar::ParseRar() {
}

ParseRar::~ParseRar() {
    close();
}

void ParseRar::close() {
    m_pageNames.clear();
    m_filePath.clear();
}

bool ParseRar::open(const std::string& path) {
    close();
    m_filePath = path;

    // Gerar diretório de cache único baseado no hash do caminho do arquivo
    unsigned int hash = 5381;
    for (char c : path) {
        hash = ((hash << 5) + hash) + (unsigned char)c;
    }
    char dirBuf[128];
    snprintf(dirBuf, sizeof(dirBuf), "ux0:data/bilingual_reader/cache/rar_%08x/", hash);
    m_cacheDir = dirBuf;
    FileUtils::makeDirRecursive(m_cacheDir);

    // Limpa cache anterior deste arquivo especifico
    SceUID dir = sceIoDopen(m_cacheDir.c_str());
    if (dir >= 0) {
        SceIoDirent dirent;
        while (sceIoDread(dir, &dirent) > 0) {
            std::string file = m_cacheDir + dirent.d_name;
            sceIoRemove(file.c_str());
        }
        sceIoDclose(dir);
    }

    if (!extractAllWithProgress()) {
        return false;
    }

    std::sort(m_pageNames.begin(), m_pageNames.end(), FileUtils::naturalSortCompare);
    return !m_pageNames.empty();
}

bool ParseRar::extractAllWithProgress() {
    RAROpenArchiveDataEx arcData = {0};
    arcData.ArcName = (char*)m_filePath.c_str();
    arcData.OpenMode = RAR_OM_EXTRACT;
    
    HANDLE hArc = RAROpenArchiveEx(&arcData);
    if (!hArc || arcData.OpenResult != ERAR_SUCCESS) return false;

    RARSetCallback(hArc, UnrarCallback, 0);

    // Primeiro contar total de arquivos de imagem para o progresso
    int totalImages = 0;
    RARHeaderDataEx headerData = {0};
    while (RARReadHeaderEx(hArc, &headerData) == ERAR_SUCCESS) {
        if (FileUtils::isImageFile(headerData.FileName)) {
            totalImages++;
        }
        RARProcessFile(hArc, RAR_SKIP, NULL, NULL);
    }
    RARCloseArchive(hArc);

    if (totalImages == 0) return false;

    // Reabrir para extracao
    hArc = RAROpenArchiveEx(&arcData);
    if (!hArc || arcData.OpenResult != ERAR_SUCCESS) return false;

    int extracted = 0;
    memset(&headerData, 0, sizeof(headerData));

    while (RARReadHeaderEx(hArc, &headerData) == ERAR_SUCCESS) {
        bool isImage = FileUtils::isImageFile(headerData.FileName);
        
        if (isImage) {
            std::string ext = FileUtils::getExtension(headerData.FileName);
            if (ext.empty()) ext = "jpg";
            
            char formattedName[64];
            snprintf(formattedName, sizeof(formattedName), "page_%05d.%s", extracted + 1, ext.c_str());
            
            std::string outPath = m_cacheDir + formattedName;
            
            int res = RARProcessFile(hArc, RAR_EXTRACT, NULL, (char*)outPath.c_str());
            if (res == ERAR_SUCCESS) {
                m_pageNames.push_back(outPath);
                extracted++;
            }
            
            // Desenha o progresso apenas a cada 5 páginas ou no início/fim para evitar timeout de semáforo gráfico
            if (extracted == 1 || extracted == totalImages || (extracted % 5 == 0)) {
                float progress = (float)extracted / (float)totalImages;
                vita2d_start_drawing();
                vita2d_clear_screen();
                UIComponents::drawProgressPopup("Extraindo", "Descompactando...", progress);
                vita2d_end_drawing();
                vita2d_swap_buffers();
            }
        } else {
            RARProcessFile(hArc, RAR_SKIP, NULL, NULL);
        }
    }
    
    RARCloseArchive(hArc);
    return true;
}

const std::vector<std::string>& ParseRar::getPages() const {
    return m_pageNames;
}

size_t ParseRar::getPageCount() const {
    return m_pageNames.size();
}

bool ParseRar::extractPage(size_t index, const std::string& outPath) {
    if (index >= m_pageNames.size()) return false;
    
    // Como ja extraimos tudo, apenas copiamos o arquivo do cache
    return FileUtils::copyFile(m_pageNames[index], outPath);
}

bool ParseRar::extractCover(const std::string& outPath) {
    if (m_pageNames.empty()) return false;
    return extractPage(0, outPath);
}

vita2d_texture* ParseRar::loadPageTexture(size_t index) {
    if (index >= m_pageNames.size()) return nullptr;
    
    std::string path = m_pageNames[index];
    return ImageLoader::loadTextureFromFile(path);
}

vita2d_texture* ParseRar::loadPageThumbnail(size_t index, int maxW, int maxH) {
    if (index >= m_pageNames.size()) return nullptr;

    std::string path = m_pageNames[index];
    return ImageLoader::loadThumbnailFromFile(path, maxW, maxH);
}

bool ParseRar::isDoublePage(size_t index) {
    if (index >= m_pageNames.size()) return false;
    int w = 0, h = 0;
    if (ImageLoader::getImageDimensionsFromFile(m_pageNames[index], w, h)) {
        return (w > 0 && h > 0 && ((float)w / (float)h) > 1.15f);
    }
    return false;
}

bool ParseRar::extractFirstImage(const std::string& rarPath, const std::string& outImagePath) {
    if (rarPath.empty() || outImagePath.empty()) return false;

    RAROpenArchiveDataEx arcData = {0};
    arcData.ArcName = (char*)rarPath.c_str();
    arcData.OpenMode = RAR_OM_EXTRACT;

    HANDLE hArc = RAROpenArchiveEx(&arcData);
    if (!hArc || arcData.OpenResult != ERAR_SUCCESS) return false;

    RARHeaderDataEx headerData = {0};
    bool extracted = false;

    while (RARReadHeaderEx(hArc, &headerData) == ERAR_SUCCESS) {
        if (FileUtils::isImageFile(headerData.FileName)) {
            int res = RARProcessFile(hArc, RAR_EXTRACT, NULL, (char*)outImagePath.c_str());
            if (res == ERAR_SUCCESS) {
                extracted = true;
                break;
            }
        }
        RARProcessFile(hArc, RAR_SKIP, NULL, NULL);
    }

    RARCloseArchive(hArc);
    return extracted;
}
