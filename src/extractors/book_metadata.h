#pragma once

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
};

