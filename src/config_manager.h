#pragma once
#include <string>
#include "file_browser.h"

struct AppConfig {
    SortMode sortMode = SortMode::NAME;
    bool isGridView = false;
    bool readerRotated = false;
    bool showPageNumbers = true;
    int epubFontSize = 24;
};

class ConfigManager {
public:
    static ConfigManager& getInstance();

    void load();
    void save();

    AppConfig& getConfig() { return m_config; }
    const AppConfig& getConfig() const { return m_config; }

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    AppConfig m_config;
    std::string m_configPath = "ux0:data/bilingual_reader/config.ini";
};
