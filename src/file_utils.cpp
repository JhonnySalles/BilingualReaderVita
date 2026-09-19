#include "file_utils.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sstream>

#ifdef __vita__
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace FileUtils {

std::string getExtension(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) return "";
    std::string ext = path.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return ext;
}

std::string getFileName(const std::string& path) {
    size_t slashPos = path.find_last_of("/\\");
    if (slashPos == std::string::npos) return path;
    return path.substr(slashPos + 1);
}

bool isImageFile(const std::string& filename) {
    std::string ext = getExtension(filename);
    return (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "webp");
}

bool naturalSortCompare(const std::string& a, const std::string& b) {
    size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (std::isdigit(static_cast<unsigned char>(a[i])) && std::isdigit(static_cast<unsigned char>(b[j]))) {
            size_t i_start = i;
            while (i < a.size() && std::isdigit(static_cast<unsigned char>(a[i]))) i++;
            unsigned long numA = std::strtoul(a.substr(i_start, i - i_start).c_str(), nullptr, 10);

            size_t j_start = j;
            while (j < b.size() && std::isdigit(static_cast<unsigned char>(b[j]))) j++;
            unsigned long numB = std::strtoul(b.substr(j_start, j - j_start).c_str(), nullptr, 10);

            if (numA != numB) return numA < numB;
        } else {
            char ca = std::tolower(static_cast<unsigned char>(a[i]));
            char cb = std::tolower(static_cast<unsigned char>(b[j]));
            if (ca != cb) return ca < cb;
            i++;
            j++;
        }
    }
    return a.size() < b.size();
}

bool makeDir(const std::string& path) {
#ifdef __vita__
    sceIoMkdir(path.c_str(), 0777);
    return true;
#else
    #ifdef _WIN32
    return (mkdir(path.c_str()) == 0);
    #else
    return (mkdir(path.c_str(), 0777) == 0);
    #endif
#endif
}

bool makeDirRecursive(const std::string& path) {
    std::string current;
    for (size_t i = 0; i < path.size(); ++i) {
        char c = path[i];
        current += c;
        if (c == '/' || c == '\\' || i == path.size() - 1) {
            if (current.find(':') != std::string::npos && current.find('/') == std::string::npos && current.find('\\') == std::string::npos) {
                continue; // drive letter or ux0:
            }
            makeDir(current);
        }
    }
    return true;
}

bool removeDirRecursive(const std::string& path) {
#ifdef __vita__
    SceUID dfd = sceIoDopen(path.c_str());
    if (dfd >= 0) {
        SceIoDirent dir;
        while (sceIoDread(dfd, &dir) > 0) {
            if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0) continue;
            std::string subPath = path + "/" + dir.d_name;
            if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
                removeDirRecursive(subPath);
            } else {
                std::remove(subPath.c_str());
            }
        }
        sceIoDclose(dfd);
        sceIoRmdir(path.c_str());
    }
    return true;
#else
    // Fallback stub for desktop test
    return true;
#endif
}

std::string generateRandomId(size_t length) {
    static const char chars[] = "0123456789abcdefghijklmnopqrstuvwxyz";
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }
    std::string id;
    id.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        id += chars[std::rand() % (sizeof(chars) - 1)];
    }
    return id;
}

bool readMagicBytes(const std::string& path, unsigned char* buffer, size_t size) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    size_t readCount = fread(buffer, 1, size, f);
    fclose(f);
    return (readCount == size);
}

} // namespace FileUtils
