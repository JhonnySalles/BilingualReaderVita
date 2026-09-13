#include "file_browser.h"
#include <psp2/io/dirent.h>
#include <psp2/io/stat.h>
#include <psp2/kernel/processmgr.h>
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <ctime>

const std::string FileBrowser::BASE_DIRECTORY = "ux0:data/BilingualReaderVita/";
const std::string FileBrowser::META_FILE = "ux0:data/BilingualReaderVita/library_meta.dat";

void FileBrowser::ensureDirectoryExists() {
    sceIoMkdir("ux0:data", 0777);
    sceIoMkdir(BASE_DIRECTORY.c_str(), 0777);
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

static FileType getFileType(const std::string& name) {
    if (name.length() < 4) return FileType::UNKNOWN;
    
    size_t dotPos = name.find_last_of(".");
    if (dotPos == std::string::npos) return FileType::UNKNOWN;
    
    std::string ext = name.substr(dotPos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "txt") return FileType::TXT;
    if (ext == "cbz") return FileType::CBZ;
    if (ext == "epub") return FileType::EPUB;
    
    return FileType::UNKNOWN;
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

void FileBrowser::loadMetadata(std::vector<LibraryItem>& items) {
    std::ifstream in(META_FILE);
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
    std::ofstream out(META_FILE);
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

void FileBrowser::sortItems(std::vector<LibraryItem>& items, SortMode mode) {
    switch (mode) {
        case SortMode::NAME:
            std::sort(items.begin(), items.end(), [](const LibraryItem& a, const LibraryItem& b) {
                std::string na = a.filename;
                std::string nb = b.filename;
                std::transform(na.begin(), na.end(), na.begin(), ::tolower);
                std::transform(nb.begin(), nb.end(), nb.begin(), ::tolower);
                return na < nb;
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
                return a.filename < b.filename;
            });
            break;
        case SortMode::TYPE:
            std::sort(items.begin(), items.end(), [](const LibraryItem& a, const LibraryItem& b) {
                if (a.typeString != b.typeString) {
                    return a.typeString < b.typeString;
                }
                return a.filename < b.filename;
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

    SceUID dfd = sceIoDopen(BASE_DIRECTORY.c_str());
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
            item.fullPath = BASE_DIRECTORY + filename;
            item.type = type;
            item.fileSize = dir.d_stat.st_size;
            item.formattedSize = formatFileSize(item.fileSize);
            item.isFavorite = false;
            item.lastReadTime = 0;

            switch (type) {
                case FileType::TXT: item.typeString = "TXT"; break;
                case FileType::CBZ: item.typeString = "CBZ"; break;
                case FileType::EPUB: item.typeString = "EPUB"; break;
                default: item.typeString = "???"; break;
            }

            items.push_back(item);
        }
    }

    sceIoDclose(dfd);

    loadMetadata(items);
    sortItems(items, SortMode::NAME);

    return items;
}
