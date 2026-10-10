#pragma once
#include <string>
#include "file_browser.h"
#include "theme_manager.h"

enum class ReaderScrollMode {
    PagedHorizontal = 0,
    PagedVertical = 1,
    InfiniteScroll = 2,
    COUNT = 3
};

enum class PageTransition {
    Slide = 0,
    Fade = 1,
    Stack = 2,
    COUNT = 3
};

// Duração das animações de página / passo de rolagem infinita (segundos).
constexpr float kReaderTransitionDuration = 0.5f;

inline const char* readerScrollModeName(ReaderScrollMode mode, bool shortLabel = false) {
    switch (mode) {
        case ReaderScrollMode::PagedVertical:
            return shortLabel ? "< Paginado Vert. >" : "< Paginado Vertical >";
        case ReaderScrollMode::InfiniteScroll:
            return "< Rolagem infinita >";
        case ReaderScrollMode::PagedHorizontal:
        default:
            return shortLabel ? "< Paginado Horiz. >" : "< Paginado Horizontal >";
    }
}

inline const char* pageTransitionName(PageTransition mode, bool shortLabel = false) {
    switch (mode) {
        case PageTransition::Fade:
            return "< Fade >";
        case PageTransition::Stack:
            return shortLabel ? "< Empilhar >" : "< Empilhar >";
        case PageTransition::Slide:
        default:
            return shortLabel ? "< Deslizar >" : "< Deslizar >";
    }
}

struct AppConfig {
    SortMode sortMode = SortMode::NAME;
    bool isGridView = false;
    bool readerRotated = false;
    ReaderScrollMode readerScrollMode = ReaderScrollMode::PagedHorizontal;
    PageTransition pageTransition = PageTransition::Slide;
    bool showPageNumbers = true;
    int epubFontSize = 24;
    ThemeId theme = ThemeId::DEFAULT;
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
