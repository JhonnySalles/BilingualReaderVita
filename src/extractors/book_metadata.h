#pragma once

#include "detail_metadata.h"
#include <string>
#include <vector>

struct BookMetadata {
    std::string title;
    std::string author;
    std::string filePath;
    std::string annotation;
    std::string publisher;
    std::string genre;
    std::string language;
    std::string releaseDate;
    std::string series;
    int seriesIndex = 0;
    std::string isbn;
    std::vector<std::string> subjects;
    std::vector<ChapterItem> chapters;
};
