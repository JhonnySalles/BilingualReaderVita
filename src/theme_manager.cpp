#include "theme_manager.h"
#include "ui_components.h"
#include <vita2d.h>

#define COLOR_RGBA(r, g, b, a) ((unsigned int)(((a) << 24) | ((b) << 16) | ((g) << 8) | (r)))

// Definições globais das variáveis mutáveis de UITheme
namespace UITheme {
    unsigned int Background      = COLOR_RGBA(10, 10, 10, 255);
    unsigned int TopBar          = COLOR_RGBA(0, 0, 0, 255);
    unsigned int Surface         = COLOR_RGBA(22, 22, 22, 255);
    unsigned int SurfaceActive   = COLOR_RGBA(35, 35, 35, 255);
    unsigned int Divider         = COLOR_RGBA(44, 44, 44, 255);
    unsigned int Primary         = COLOR_RGBA(118, 185, 0, 255);   // Verde NVIDIA #76B900
    unsigned int OnPrimary       = COLOR_RGBA(0, 0, 0, 255);       // Texto preto sobre NVIDIA Green
    unsigned int Secondary       = COLOR_RGBA(155, 226, 45, 255);  // #9BE22D
    unsigned int TextPrimary     = COLOR_RGBA(242, 242, 242, 255);
    unsigned int TextSecondary   = COLOR_RGBA(154, 154, 154, 255);
    unsigned int SearchBarBg     = COLOR_RGBA(28, 28, 28, 255);
    unsigned int SearchBarActive = COLOR_RGBA(38, 38, 38, 255);
    unsigned int ProgressBarBg   = COLOR_RGBA(28, 28, 28, 200);
    unsigned int CoverBg         = COLOR_RGBA(18, 18, 18, 255);
    unsigned int CoverBgActive   = COLOR_RGBA(30, 30, 30, 255);
}

static ThemeId s_currentTheme = ThemeId::DEFAULT;

static const ThemePalette s_palettes[static_cast<int>(ThemeId::COUNT)] = {
    // 0: DEFAULT (NVIDIA Green + Black)
    {
        COLOR_RGBA(10, 10, 10, 255),      // Background (#0A0A0A)
        COLOR_RGBA(0, 0, 0, 255),        // TopBar (#000000)
        COLOR_RGBA(22, 22, 22, 255),      // Surface (#161616)
        COLOR_RGBA(35, 35, 35, 255),      // SurfaceActive (#232323)
        COLOR_RGBA(44, 44, 44, 255),      // Divider (#2C2C2C)
        COLOR_RGBA(118, 185, 0, 255),     // Primary (#76B900)
        COLOR_RGBA(0, 0, 0, 255),        // OnPrimary (#000000)
        COLOR_RGBA(155, 226, 45, 255),    // Secondary (#9BE22D)
        COLOR_RGBA(242, 242, 242, 255),  // TextPrimary
        COLOR_RGBA(154, 154, 154, 255),  // TextSecondary
        COLOR_RGBA(28, 28, 28, 255),      // SearchBarBg
        COLOR_RGBA(38, 38, 38, 255),      // SearchBarActive
        COLOR_RGBA(28, 28, 28, 200),      // ProgressBarBg
        COLOR_RGBA(18, 18, 18, 255),      // CoverBg
        COLOR_RGBA(30, 30, 30, 255)       // CoverBgActive
    },
    // 1: BLUE (Azul)
    {
        COLOR_RGBA(11, 14, 26, 255),      // Background (#0B0E1A)
        COLOR_RGBA(7, 9, 18, 255),        // TopBar (#070912)
        COLOR_RGBA(20, 26, 46, 255),      // Surface (#141A2E)
        COLOR_RGBA(31, 40, 71, 255),      // SurfaceActive (#1F2847)
        COLOR_RGBA(40, 50, 88, 255),      // Divider (#283258)
        COLOR_RGBA(77, 105, 230, 255),    // Primary (#4D69E6)
        COLOR_RGBA(255, 255, 255, 255),  // OnPrimary
        COLOR_RGBA(186, 195, 255, 255),  // Secondary (#BAC3FF)
        COLOR_RGBA(240, 242, 250, 255),  // TextPrimary
        COLOR_RGBA(150, 158, 185, 255),  // TextSecondary
        COLOR_RGBA(24, 30, 52, 255),      // SearchBarBg
        COLOR_RGBA(36, 46, 78, 255),      // SearchBarActive
        COLOR_RGBA(24, 30, 52, 200),      // ProgressBarBg
        COLOR_RGBA(16, 20, 36, 255),      // CoverBg
        COLOR_RGBA(28, 36, 62, 255)       // CoverBgActive
    },
    // 2: OCEAN_BLUE (Azul Oceano)
    {
        COLOR_RGBA(5, 11, 20, 255),       // Background (#050B14)
        COLOR_RGBA(2, 6, 13, 255),        // TopBar (#02060D)
        COLOR_RGBA(13, 24, 40, 255),      // Surface (#0D1828)
        COLOR_RGBA(20, 38, 64, 255),      // SurfaceActive (#142640)
        COLOR_RGBA(29, 52, 84, 255),      // Divider (#1D3454)
        COLOR_RGBA(30, 144, 255, 255),    // Primary (#1E90FF)
        COLOR_RGBA(0, 0, 0, 255),        // OnPrimary
        COLOR_RGBA(127, 212, 255, 255),   // Secondary (#7FD4FF)
        COLOR_RGBA(235, 245, 255, 255),  // TextPrimary
        COLOR_RGBA(140, 165, 195, 255),  // TextSecondary
        COLOR_RGBA(16, 30, 48, 255),      // SearchBarBg
        COLOR_RGBA(24, 44, 70, 255),      // SearchBarActive
        COLOR_RGBA(16, 30, 48, 200),      // ProgressBarBg
        COLOR_RGBA(10, 18, 30, 255),      // CoverBg
        COLOR_RGBA(18, 32, 52, 255)       // CoverBgActive
    },
    // 3: GREEN (Verde)
    {
        COLOR_RGBA(11, 18, 11, 255),      // Background (#0B120B)
        COLOR_RGBA(6, 10, 6, 255),        // TopBar (#060A06)
        COLOR_RGBA(20, 32, 20, 255),      // Surface (#142014)
        COLOR_RGBA(30, 48, 30, 255),      // SurfaceActive (#1E301E)
        COLOR_RGBA(39, 64, 39, 255),      // Divider (#274027)
        COLOR_RGBA(67, 160, 71, 255),     // Primary (#43A047)
        COLOR_RGBA(0, 0, 0, 255),        // OnPrimary
        COLOR_RGBA(129, 219, 116, 255),   // Secondary (#81DB74)
        COLOR_RGBA(240, 248, 240, 255),  // TextPrimary
        COLOR_RGBA(150, 175, 150, 255),  // TextSecondary
        COLOR_RGBA(24, 38, 24, 255),      // SearchBarBg
        COLOR_RGBA(34, 54, 34, 255),      // SearchBarActive
        COLOR_RGBA(24, 38, 24, 200),      // ProgressBarBg
        COLOR_RGBA(16, 26, 16, 255),      // CoverBg
        COLOR_RGBA(26, 42, 26, 255)       // CoverBgActive
    },
    // 4: FOREST_GREEN (Verde Floresta)
    {
        COLOR_RGBA(11, 16, 10, 255),      // Background (#0B100A)
        COLOR_RGBA(6, 9, 5, 255),         // TopBar (#060905)
        COLOR_RGBA(23, 34, 21, 255),      // Surface (#172215)
        COLOR_RGBA(34, 50, 31, 255),      // SurfaceActive (#22321F)
        COLOR_RGBA(45, 66, 41, 255),      // Divider (#2D4229)
        COLOR_RGBA(46, 158, 58, 255),     // Primary (#2E9E3A)
        COLOR_RGBA(0, 0, 0, 255),        // OnPrimary
        COLOR_RGBA(163, 211, 151, 255),   // Secondary (#A3D397)
        COLOR_RGBA(240, 247, 238, 255),  // TextPrimary
        COLOR_RGBA(152, 172, 148, 255),  // TextSecondary
        COLOR_RGBA(26, 38, 24, 255),      // SearchBarBg
        COLOR_RGBA(36, 54, 33, 255),      // SearchBarActive
        COLOR_RGBA(26, 38, 24, 200),      // ProgressBarBg
        COLOR_RGBA(17, 26, 16, 255),      // CoverBg
        COLOR_RGBA(28, 42, 26, 255)       // CoverBgActive
    },
    // 5: PINK (Rosa)
    {
        COLOR_RGBA(20, 15, 16, 255),      // Background (#140F10)
        COLOR_RGBA(12, 8, 9, 255),        // TopBar (#0C0809)
        COLOR_RGBA(34, 26, 27, 255),      // Surface (#221A1B)
        COLOR_RGBA(51, 39, 42, 255),      // SurfaceActive (#33272A)
        COLOR_RGBA(64, 48, 52, 255),      // Divider (#403034)
        COLOR_RGBA(232, 160, 168, 255),   // Primary (#E8A0A8)
        COLOR_RGBA(43, 26, 28, 255),      // OnPrimary (#2B1A1C)
        COLOR_RGBA(255, 223, 219, 255),   // Secondary (#FFDFDB)
        COLOR_RGBA(250, 240, 242, 255),  // TextPrimary
        COLOR_RGBA(180, 155, 160, 255),  // TextSecondary
        COLOR_RGBA(38, 28, 30, 255),      // SearchBarBg
        COLOR_RGBA(54, 42, 44, 255),      // SearchBarActive
        COLOR_RGBA(38, 28, 30, 200),      // ProgressBarBg
        COLOR_RGBA(26, 20, 21, 255),      // CoverBg
        COLOR_RGBA(42, 32, 34, 255)       // CoverBgActive
    },
    // 6: RED (Vermelho)
    {
        COLOR_RGBA(18, 10, 10, 255),      // Background (#120A0A)
        COLOR_RGBA(10, 5, 5, 255),        // TopBar (#0A0505)
        COLOR_RGBA(31, 20, 20, 255),      // Surface (#1F1414)
        COLOR_RGBA(46, 28, 28, 255),      // SurfaceActive (#2E1C1C)
        COLOR_RGBA(59, 37, 37, 255),      // Divider (#3B2525)
        COLOR_RGBA(229, 57, 53, 255),     // Primary (#E53935)
        COLOR_RGBA(255, 255, 255, 255),  // OnPrimary
        COLOR_RGBA(255, 180, 168, 255),   // Secondary (#FFB4A8)
        COLOR_RGBA(250, 240, 240, 255),  // TextPrimary
        COLOR_RGBA(185, 150, 150, 255),  // TextSecondary
        COLOR_RGBA(36, 22, 22, 255),      // SearchBarBg
        COLOR_RGBA(50, 32, 32, 255),      // SearchBarActive
        COLOR_RGBA(36, 22, 22, 200),      // ProgressBarBg
        COLOR_RGBA(24, 14, 14, 255),      // CoverBg
        COLOR_RGBA(38, 24, 24, 255)       // CoverBgActive
    },
    // 7: BLOOD_RED (Vermelho Sangue)
    {
        COLOR_RGBA(13, 5, 5, 255),        // Background (#0D0505)
        COLOR_RGBA(7, 2, 2, 255),         // TopBar (#070202)
        COLOR_RGBA(26, 11, 10, 255),      // Surface (#1A0B0A)
        COLOR_RGBA(42, 17, 16, 255),      // SurfaceActive (#2A1110)
        COLOR_RGBA(58, 23, 20, 255),      // Divider (#3A1714)
        COLOR_RGBA(179, 20, 11, 255),     // Primary (#B3140B)
        COLOR_RGBA(255, 255, 255, 255),  // OnPrimary
        COLOR_RGBA(255, 138, 128, 255),   // Secondary (#FF8A80)
        COLOR_RGBA(250, 238, 238, 255),  // TextPrimary
        COLOR_RGBA(185, 145, 145, 255),  // TextSecondary
        COLOR_RGBA(30, 14, 13, 255),      // SearchBarBg
        COLOR_RGBA(45, 20, 18, 255),      // SearchBarActive
        COLOR_RGBA(30, 14, 13, 200),      // ProgressBarBg
        COLOR_RGBA(20, 8, 8, 255),        // CoverBg
        COLOR_RGBA(34, 14, 13, 255)       // CoverBgActive
    }
};

static const char* s_themeNames[static_cast<int>(ThemeId::COUNT)] = {
    "Padrão",
    "Azul",
    "Azul Oceano",
    "Verde",
    "Verde Floresta",
    "Rosa",
    "Vermelho",
    "Vermelho Sangue"
};

static const char* s_themeKeys[static_cast<int>(ThemeId::COUNT)] = {
    "DEFAULT",
    "BLUE",
    "OCEAN_BLUE",
    "GREEN",
    "FOREST_GREEN",
    "PINK",
    "RED",
    "BLOOD_RED"
};

void ThemeManager::apply(ThemeId id) {
    int idx = static_cast<int>(id);
    if (idx < 0 || idx >= static_cast<int>(ThemeId::COUNT)) {
        idx = 0;
        id = ThemeId::DEFAULT;
    }
    s_currentTheme = id;
    const ThemePalette& p = s_palettes[idx];

    UITheme::Background      = p.Background;
    UITheme::TopBar          = p.TopBar;
    UITheme::Surface         = p.Surface;
    UITheme::SurfaceActive   = p.SurfaceActive;
    UITheme::Divider         = p.Divider;
    UITheme::Primary         = p.Primary;
    UITheme::OnPrimary       = p.OnPrimary;
    UITheme::Secondary       = p.Secondary;
    UITheme::TextPrimary     = p.TextPrimary;
    UITheme::TextSecondary   = p.TextSecondary;
    UITheme::SearchBarBg     = p.SearchBarBg;
    UITheme::SearchBarActive = p.SearchBarActive;
    UITheme::ProgressBarBg   = p.ProgressBarBg;
    UITheme::CoverBg         = p.CoverBg;
    UITheme::CoverBgActive   = p.CoverBgActive;

    vita2d_set_clear_color(p.Background);
}

ThemeId ThemeManager::current() {
    return s_currentTheme;
}

const ThemePalette& ThemeManager::palette(ThemeId id) {
    int idx = static_cast<int>(id);
    if (idx < 0 || idx >= static_cast<int>(ThemeId::COUNT)) {
        idx = 0;
    }
    return s_palettes[idx];
}

const char* ThemeManager::displayName(ThemeId id) {
    int idx = static_cast<int>(id);
    if (idx < 0 || idx >= static_cast<int>(ThemeId::COUNT)) {
        idx = 0;
    }
    return s_themeNames[idx];
}

const char* ThemeManager::key(ThemeId id) {
    int idx = static_cast<int>(id);
    if (idx < 0 || idx >= static_cast<int>(ThemeId::COUNT)) {
        idx = 0;
    }
    return s_themeKeys[idx];
}

ThemeId ThemeManager::fromKey(const std::string& k) {
    for (int i = 0; i < static_cast<int>(ThemeId::COUNT); i++) {
        if (k == s_themeKeys[i]) {
            return static_cast<ThemeId>(i);
        }
    }
    return ThemeId::DEFAULT;
}
