#pragma once
#include <string>
#include <vector>
#include <cstdint>

enum class FileType {
    TXT,
    CBZ,
    CBR,
    CBT,
    CB7,
    ZIP,
    RAR,
    TAR,
    SEVEN_ZIP,
    EPUB,
    UNKNOWN
};

enum class SortMode {
    NAME = 0,
    LAST_READ,
    FAVORITES,
    TYPE,
    COUNT
};

struct LibraryItem {
    std::string filename;
    std::string fullPath;
    FileType type;
    std::string typeString;
    size_t fileSize;
    std::string formattedSize;
    bool isFavorite = false;
    uint64_t lastReadTime = 0;
};

class FileBrowser {
public:
    static void ensureDirectoryExists();
    static std::vector<LibraryItem> scanLibrary();
    static std::string formatFileSize(size_t bytes);

    static void sortItems(std::vector<LibraryItem>& items, SortMode mode);
    static std::vector<LibraryItem> filterItems(const std::vector<LibraryItem>& items, const std::string& query);
    static const char* getSortModeName(SortMode mode);

    static void loadMetadata(std::vector<LibraryItem>& items);
    static void saveMetadata(const std::vector<LibraryItem>& items);
    static void markAsRead(LibraryItem& item);
    static void toggleFavorite(LibraryItem& item);
    static bool deleteItem(const LibraryItem& item);

    static FileType getFileType(const std::string& filename);
    static bool isMangaType(FileType type);
};
