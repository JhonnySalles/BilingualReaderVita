#include "parse_rar.h"
#include "../file_utils.h"
#include "../ui_components.h"

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
    m_cacheDir = "ux0:data/bilingual_reader/cache/rar_extracted/";
    FileUtils::makeDirRecursive(m_cacheDir); // Garante que toda a árvore de diretórios existe
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

    // Limpa cache anterior
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
            std::string outName = headerData.FileName;
            std::replace(outName.begin(), outName.end(), '/', '_');
            std::replace(outName.begin(), outName.end(), '\\', '_');
            
            std::string outPath = m_cacheDir + outName;
            
            int res = RARProcessFile(hArc, RAR_EXTRACT, NULL, (char*)outPath.c_str());
            if (res == ERAR_SUCCESS) {
                m_pageNames.push_back(outPath);
                extracted++;
            }
            
            // Desenha o progresso
            float progress = (float)extracted / (float)totalImages;
            vita2d_start_drawing();
            vita2d_clear_screen();
            UIComponents::drawProgressPopup("Extraindo", "Descompactando...", progress);
            vita2d_end_drawing();
            vita2d_swap_buffers();
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
    std::string ext = FileUtils::getExtension(path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "png") {
        return vita2d_load_PNG_file(path.c_str());
    } else if (ext == "jpg" || ext == "jpeg") {
        return vita2d_load_JPEG_file(path.c_str());
    }
    return nullptr;
}
