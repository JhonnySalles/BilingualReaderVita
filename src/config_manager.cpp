#include "config_manager.h"
#include "file_utils.h"
#include <fstream>
#include <sstream>
#include <iostream>

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::load() {
    FileUtils::makeDirRecursive("ux0:data/bilingual_reader");
    std::ifstream file(m_configPath);
    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = line.substr(0, eqPos);
        std::string val = line.substr(eqPos + 1);

        // Trim
        while (!key.empty() && (key.back() == ' ' || key.back() == '\r' || key.back() == '\t')) key.pop_back();
        while (!val.empty() && (val.back() == ' ' || val.back() == '\r' || val.back() == '\t')) val.pop_back();

        if (key == "sort_mode") {
            int mode = std::stoi(val);
            if (mode >= 0 && mode < static_cast<int>(SortMode::COUNT)) {
                m_config.sortMode = static_cast<SortMode>(mode);
            }
        } else if (key == "grid_view") {
            m_config.isGridView = (val == "1" || val == "true");
        } else if (key == "reader_rotated") {
            m_config.readerRotated = (val == "1" || val == "true");
        } else if (key == "show_page_numbers") {
            m_config.showPageNumbers = (val == "1" || val == "true");
        } else if (key == "epub_font_size") {
            int sz = std::stoi(val);
            if (sz >= 12 && sz <= 48) {
                m_config.epubFontSize = sz;
            }
        }
    }
    file.close();
}

void ConfigManager::save() {
    FileUtils::makeDirRecursive("ux0:data/bilingual_reader");
    std::ofstream file(m_configPath);
    if (!file.is_open()) {
        return;
    }

    file << "[BilingualReader]\n";
    file << "sort_mode=" << static_cast<int>(m_config.sortMode) << "\n";
    file << "grid_view=" << (m_config.isGridView ? "1" : "0") << "\n";
    file << "reader_rotated=" << (m_config.readerRotated ? "1" : "0") << "\n";
    file << "show_page_numbers=" << (m_config.showPageNumbers ? "1" : "0") << "\n";
    file << "epub_font_size=" << m_config.epubFontSize << "\n";

    file.close();
}
