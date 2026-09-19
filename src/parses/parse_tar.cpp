#include "parse_tar.h"
#include "../file_utils.h"
#include <algorithm>
#include <cstring>
#include <cstdlib>

struct TarHeader {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
};

static size_t parseOctal(const char* str, size_t maxLen) {
    size_t val = 0;
    while (maxLen > 0 && *str == ' ') {
        str++;
        maxLen--;
    }
    while (maxLen > 0 && *str >= '0' && *str <= '7') {
        val = (val << 3) | (*str - '0');
        str++;
        maxLen--;
    }
    return val;
}

ParseTar::ParseTar() : m_file(nullptr) {
}

ParseTar::~ParseTar() {
    close();
}

void ParseTar::close() {
    if (m_file) {
        fclose(m_file);
        m_file = nullptr;
    }
    m_pageNames.clear();
    m_entries.clear();
    m_filePath.clear();
}

bool ParseTar::open(const std::string& path) {
    close();
    m_filePath = path;
    m_file = fopen(path.c_str(), "rb");
    if (!m_file) return false;

    TarHeader header;
    while (fread(&header, 1, 512, m_file) == 512) {
        if (header.name[0] == '\0') {
            break; // Fim do arquivo TAR (blocos nulos)
        }

        size_t fileSize = parseOctal(header.size, sizeof(header.size));
        long dataOffset = ftell(m_file);

        std::string entryName(header.name);
        if (header.prefix[0] != '\0') {
            entryName = std::string(header.prefix) + "/" + entryName;
        }

        // Filtra apenas arquivos normais (typeflag '0' ou '\0') e que sejam imagens
        if ((header.typeflag == '0' || header.typeflag == '\0') && FileUtils::isImageFile(entryName)) {
            // Ignora arquivos ocultos/lixo do macOS
            if (entryName.find("__MACOSX") == std::string::npos && entryName.find("/.") == std::string::npos) {
                TarEntry entry;
                entry.name = entryName;
                entry.dataOffset = dataOffset;
                entry.size = fileSize;

                m_entries[entryName] = entry;
                m_pageNames.push_back(entryName);
            }
        }

        // Pula os dados do arquivo alinhados em blocos de 512 bytes
        size_t blocks = (fileSize + 511) / 512;
        fseek(m_file, dataOffset + (blocks * 512), SEEK_SET);
    }

    if (m_pageNames.empty()) {
        close();
        return false;
    }

    std::sort(m_pageNames.begin(), m_pageNames.end(), FileUtils::naturalSortCompare);
    return true;
}

const std::vector<std::string>& ParseTar::getPages() const {
    return m_pageNames;
}

size_t ParseTar::getPageCount() const {
    return m_pageNames.size();
}

bool ParseTar::extractPage(size_t index, const std::string& outPath) {
    if (index >= m_pageNames.size() || !m_file) return false;

    const std::string& pageName = m_pageNames[index];
    auto it = m_entries.find(pageName);
    if (it == m_entries.end()) return false;

    const TarEntry& entry = it->second;
    fseek(m_file, entry.dataOffset, SEEK_SET);

    FILE* outF = fopen(outPath.c_str(), "wb");
    if (!outF) return false;

    char buffer[4096];
    size_t remaining = entry.size;
    while (remaining > 0) {
        size_t toRead = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
        size_t bytesRead = fread(buffer, 1, toRead, m_file);
        if (bytesRead == 0) break;
        fwrite(buffer, 1, bytesRead, outF);
        remaining -= bytesRead;
    }

    fclose(outF);
    return (remaining == 0);
}

bool ParseTar::extractCover(const std::string& outPath) {
    if (m_pageNames.empty()) return false;
    return extractPage(0, outPath);
}

vita2d_texture* ParseTar::loadPageTexture(size_t index) {
    if (index >= m_pageNames.size() || !m_file) return nullptr;

    const std::string& pageName = m_pageNames[index];
    auto it = m_entries.find(pageName);
    if (it == m_entries.end()) return nullptr;

    const TarEntry& entry = it->second;
    fseek(m_file, entry.dataOffset, SEEK_SET);

    std::vector<unsigned char> data(entry.size);
    if (fread(data.data(), 1, entry.size, m_file) != entry.size) {
        return nullptr;
    }

    std::string ext = FileUtils::getExtension(pageName);
    if (ext == "png") {
        return vita2d_load_PNG_buffer(data.data());
    } else if (ext == "jpg" || ext == "jpeg") {
        return vita2d_load_JPEG_buffer(data.data(), data.size());
    }

    return nullptr;
}
