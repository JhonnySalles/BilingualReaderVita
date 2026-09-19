#include "file_browser.h"
#include "cache_manager.h"
#include "file_utils.h"

#ifdef __vita__
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#endif

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <ctime>

void FileBrowser::ensureDirectoryExists() {
    CacheManager::getInstance().init();
}

std::string FileBrowser::formatFileSize(size_t bytes) {
    std::ostringstream ss;
    if (bytes >= 1024 * 1024) {
        ss << std::fixed << std::setprecision(1) << (static_cast<double>(bytes) / (1024.0 * 1024.0)) << " MB";
    } else if (bytes >= 1024) {
        ss << std::fixed << std::setprecision(1) << (static_cast<double>(bytes) / 1024.0) << " KB";
    } else {
        ss << bytes << " B";
    }
    return ss.str();
}

FileType FileBrowser::getFileType(const std::string& name) {
    std::string ext = FileUtils::getExtension(name);
    if (ext == "txt") return FileType::TXT;
    if (ext == "cbz") return FileType::CBZ;
    if (ext == "zip") return FileType::ZIP;
    if (ext == "cbr") return FileType::CBR;
    if (ext == "rar") return FileType::RAR;
    if (ext == "cbt") return FileType::CBT;
    if (ext == "tar") return FileType::TAR;
    if (ext == "cb7") return FileType::CB7;
    if (ext == "7z") return FileType::SEVEN_ZIP;
    if (ext == "epub") return FileType::EPUB;
    return FileType::UNKNOWN;
}

bool FileBrowser::isMangaType(FileType type) {
    switch (type) {
        case FileType::CBZ:
        case FileType::ZIP:
        case FileType::CBR:
        case FileType::RAR:
        case FileType::CBT:
        case FileType::TAR:
        case FileType::CB7:
        case FileType::SEVEN_ZIP:
            return true;
        default:
            return false;
    }
}

const char* FileBrowser::getSortModeName(SortMode mode) {
    switch (mode) {
        case SortMode::NAME: return "Nome (A-Z)";
        case SortMode::LAST_READ: return "Última Leitura";
        case SortMode::FAVORITES: return "Favoritos 1º";
        case SortMode::TYPE: return "Tipo de Arquivo";
        default: return "Padrão";
    }
}

static std::string getMetaFilePath() {
    return CacheManager::getInstance().getCacheBasePath() + "/library_meta.dat";
}

void FileBrowser::loadMetadata(std::vector<LibraryItem>& items) {
    std::ifstream in(getMetaFilePath());
    if (!in.is_open()) return;

    std::string filename;
    int fav;
    uint64_t lastRead;

    while (in >> filename >> fav >> lastRead) {
        for (auto& item : items) {
            if (item.filename == filename) {
                item.isFavorite = (fav != 0);
                item.lastReadTime = lastRead;
                break;
            }
        }
    }
    in.close();
}

void FileBrowser::saveMetadata(const std::vector<LibraryItem>& items) {
    std::ofstream out(getMetaFilePath());
    if (!out.is_open()) return;

    for (const auto& item : items) {
        out << item.filename << " " << (item.isFavorite ? 1 : 0) << " " << item.lastReadTime << "\n";
    }
    out.close();
}

void FileBrowser::markAsRead(LibraryItem& item) {
    item.lastReadTime = static_cast<uint64_t>(time(nullptr));
}

void FileBrowser::toggleFavorite(LibraryItem& item) {
    item.isFavorite = !item.isFavorite;
}

bool FileBrowser::deleteItem(const LibraryItem& item) {
    if (std::remove(item.fullPath.c_str()) != 0) {
        return false;
    }
    return true;
}

void FileBrowser::sortItems(std::vector<LibraryItem>& items, SortMode mode) {
    switch (mode) {
        case SortMode::NAME:
            std::sort(items.begin(), items.end(), [](const LibraryItem& a, const LibraryItem& b) {
                return FileUtils::naturalSortCompare(a.filename, b.filename);
            });
            break;
        case SortMode::LAST_READ:
            std::sort(items.begin(), items.end(), [](const LibraryItem& a, const LibraryItem& b) {
                return a.lastReadTime > b.lastReadTime;
            });
            break;
        case SortMode::FAVORITES:
            std::sort(items.begin(), items.end(), [](const LibraryItem& a, const LibraryItem& b) {
                if (a.isFavorite != b.isFavorite) {
                    return a.isFavorite > b.isFavorite;
                }
                return FileUtils::naturalSortCompare(a.filename, b.filename);
            });
            break;
        case SortMode::TYPE:
            std::sort(items.begin(), items.end(), [](const LibraryItem& a, const LibraryItem& b) {
                if (a.typeString != b.typeString) {
                    return a.typeString < b.typeString;
                }
                return FileUtils::naturalSortCompare(a.filename, b.filename);
            });
            break;
        default:
            break;
    }
}

std::vector<LibraryItem> FileBrowser::filterItems(const std::vector<LibraryItem>& items, const std::string& query) {
    if (query.empty()) return items;

    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    std::vector<LibraryItem> filtered;
    for (const auto& item : items) {
        std::string lowerName = item.filename;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

        if (lowerName.find(lowerQuery) != std::string::npos) {
            filtered.push_back(item);
        }
    }
    return filtered;
}

std::vector<LibraryItem> FileBrowser::scanLibrary() {
    std::vector<LibraryItem> items;
    ensureDirectoryExists();

    std::string libDir = CacheManager::getInstance().getLibraryPath();

#ifdef __vita__
    SceUID dfd = sceIoDopen(libDir.c_str());
    if (dfd < 0) {
        return items;
    }

    SceIoDirent dir;
    while (sceIoDread(dfd, &dir) > 0) {
        if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
            continue;
        }

        std::string filename = dir.d_name;
        FileType type = getFileType(filename);

        if (type != FileType::UNKNOWN) {
            LibraryItem item;
            item.filename = filename;
            item.fullPath = libDir + "/" + filename;
            item.type = type;
            item.fileSize = dir.d_stat.st_size;
            item.formattedSize = formatFileSize(item.fileSize);
            item.isFavorite = false;
            item.lastReadTime = 0;

            switch (type) {
                case FileType::TXT: item.typeString = "TXT"; break;
                case FileType::CBZ: item.typeString = "CBZ"; break;
                case FileType::ZIP: item.typeString = "ZIP"; break;
                case FileType::CBR: item.typeString = "CBR"; break;
                case FileType::RAR: item.typeString = "RAR"; break;
                case FileType::CBT: item.typeString = "CBT"; break;
                case FileType::TAR: item.typeString = "TAR"; break;
                case FileType::CB7: item.typeString = "CB7"; break;
                case FileType::SEVEN_ZIP: item.typeString = "7Z"; break;
                case FileType::EPUB: item.typeString = "EPUB"; break;
                default: item.typeString = "???"; break;
            }

            items.push_back(item);
        }
    }

    sceIoDclose(dfd);
#endif

    loadMetadata(items);
    sortItems(items, SortMode::NAME);

    return items;
}
