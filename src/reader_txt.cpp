#include "reader_txt.h"
#include "ui_components.h"
#include <fstream>
#include <sstream>
#include <cstdio>

ReaderTXT::ReaderTXT() : currentPage(0), totalPages(0) {}

ReaderTXT::~ReaderTXT() {
    close();
}

void ReaderTXT::close() {
    pages.clear();
    currentPage = 0;
    totalPages = 0;
    filename = "";
}

bool ReaderTXT::loadFile(const std::string& path, vita2d_pgf* font) {
    close();

    size_t lastSlash = path.find_last_of("/\\");
    filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;

    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string text = buffer.str();
    file.close();

    paginateText(text, font);
    return !pages.empty();
}

void ReaderTXT::paginateText(const std::string& fullText, vita2d_pgf* font) {
    const int maxLinesPerPage = 16;
    const int maxCharsPerLine = 70; // Estimativa segura para tela do Vita com PGF em escala 1.0f

    std::istringstream stream(fullText);
    std::string line;
    std::vector<std::string> formattedLines;

    while (std::getline(stream, line)) {
        if (line.empty()) {
            formattedLines.push_back("");
            continue;
        }

        while (line.length() > (size_t)maxCharsPerLine) {
            size_t splitPos = line.rfind(' ', maxCharsPerLine);
            if (splitPos == std::string::npos) {
                splitPos = maxCharsPerLine;
            }
            formattedLines.push_back(line.substr(0, splitPos));
            line = line.substr(splitPos + (line[splitPos] == ' ' ? 1 : 0));
        }
        formattedLines.push_back(line);
    }

    std::string currentPageText = "";
    int lineCounter = 0;

    for (const auto& l : formattedLines) {
        currentPageText += l + "\n";
        lineCounter++;

        if (lineCounter >= maxLinesPerPage) {
            pages.push_back(currentPageText);
            currentPageText = "";
            lineCounter = 0;
        }
    }

    if (!currentPageText.empty()) {
        pages.push_back(currentPageText);
    }

    totalPages = static_cast<int>(pages.size());
    currentPage = 0;
}

void ReaderTXT::nextPage() {
    if (currentPage < totalPages - 1) {
        currentPage++;
    }
}

void ReaderTXT::prevPage() {
    if (currentPage > 0) {
        currentPage--;
    }
}

void ReaderTXT::render(vita2d_pgf* font) {
    // Fundo do leitor
    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(18, 20, 29, 255));

    // Cabeçalho de Leitura
    vita2d_draw_rectangle(0, 0, 960, 48, UITheme::TopBar);
    vita2d_draw_line(0, 48, 960, 48, RGBA8(42, 48, 70, 255));

    if (font) {
        vita2d_pgf_draw_text(font, 32, 32, UITheme::Primary, 1.0f, filename.c_str());

        char pageInfo[64];
        snprintf(pageInfo, sizeof(pageInfo), "Pág %d / %d", currentPage + 1, totalPages > 0 ? totalPages : 1);
        vita2d_pgf_draw_text(font, 820, 32, UITheme::TextSecondary, 0.9f, pageInfo);

        // Renderiza o corpo do texto
        if (!pages.empty() && currentPage < (int)pages.size()) {
            std::istringstream stream(pages[currentPage]);
            std::string line;
            int yPos = 84;
            while (std::getline(stream, line)) {
                vita2d_pgf_draw_text(font, 48, yPos, UITheme::TextPrimary, 0.95f, line.c_str());
                yPos += 24;
            }
        }
    }

    // Rodapé
    UIComponents::drawFooter("D-Pad Esq/Dir: Navegar | O: Voltar");
}
