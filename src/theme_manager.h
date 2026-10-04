#pragma once
#include <string>

enum class ThemeId {
    DEFAULT = 0,    // Verde NVIDIA + Preto
    BLUE,           // Azul
    OCEAN_BLUE,     // Azul Oceano
    GREEN,          // Verde
    FOREST_GREEN,   // Verde Floresta
    PINK,           // Rosa
    RED,            // Vermelho
    BLOOD_RED,      // Vermelho Sangue
    COUNT
};

struct ThemePalette {
    unsigned int Background;
    unsigned int TopBar;
    unsigned int Surface;
    unsigned int SurfaceActive;
    unsigned int Divider;
    unsigned int Primary;       // Cor de destaque / seleção
    unsigned int OnPrimary;     // Cor de texto sobre Primary
    unsigned int Secondary;     // Cor secundária / cabeçalhos
    unsigned int TextPrimary;
    unsigned int TextSecondary;
    unsigned int SearchBarBg;
    unsigned int SearchBarActive;
    unsigned int ProgressBarBg;
    unsigned int CoverBg;
    unsigned int CoverBgActive;
};

class ThemeManager {
public:
    static void apply(ThemeId id);
    static ThemeId current();
    static const ThemePalette& palette(ThemeId id);
    static const char* displayName(ThemeId id);
    static const char* key(ThemeId id);
    static ThemeId fromKey(const std::string& key);
};
