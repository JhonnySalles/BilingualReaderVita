#include "reader_txt.h"
#include "ui_components.h"
#include <fstream>
#include <sstream>
#include <cstdio>

ReaderTXT::ReaderTXT() : currentPage(0), totalPages(0), isRotated(false) {}

ReaderTXT::~ReaderTXT() {
    close();
}

void ReaderTXT::close() {
    pages.clear();
    rawContent.clear();
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
    rawContent = buffer.str();
    file.close();

    paginateText(rawContent, font);
    return !pages.empty();
}

void ReaderTXT::setRotated(bool rotated, vita2d_pgf* font) {
    if (isRotated != rotated) {
        isRotated = rotated;
        int savedPage = currentPage;
        paginateText(rawContent, font);
        currentPage = std::max(0, std::min(savedPage, totalPages - 1));
    }
}

void ReaderTXT::paginateText(const std::string& fullText, vita2d_pgf* font) {
    pages.clear();
    if (fullText.empty()) {
        totalPages = 0;
        currentPage = 0;
        return;
    }

    // Modo retrato: mais linhas, menos caracteres por linha
    const int maxLinesPerPage = isRotated ? 32 : 16;
    const int maxCharsPerLine = isRotated ? 38 : 70;

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

void ReaderTXT::render(vita2d_pgf* font, bool fullscreen) {
    // Fundo do leitor
    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(18, 20, 29, 255));

    if (!fullscreen) {
        char pageInfo[64];
        snprintf(pageInfo, sizeof(pageInfo), "Pag %d / %d", currentPage + 1, totalPages > 0 ? totalPages : 1);
        UIComponents::drawReaderTopBar(filename, pageInfo, isRotated, UITheme::BadgeTXT);
    }

    if (font) {
        // Renderiza o corpo do texto
        if (!pages.empty() && currentPage < (int)pages.size()) {
            std::istringstream stream(pages[currentPage]);
            std::string line;
            int yPos = fullscreen ? 36 : 84;
            int xPos = isRotated ? 240 : 48;
            while (std::getline(stream, line)) {
                vita2d_pgf_draw_text(font, (float)xPos, (float)yPos, UITheme::TextPrimary, 0.95f, line.c_str());
                yPos += 24;
            }
        }
    }

    if (!fullscreen) {
        UIComponents::drawReaderProgressBar(currentPage, totalPages, false);
        UIComponents::drawFooter("D-Pad: Paginas | O: Voltar", true);
    }
}
