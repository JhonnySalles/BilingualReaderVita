#include "reader_epub.h"
#include "ui_components.h"
#include <fstream>
#include <sstream>
#include <cstdio>

ReaderEPUB::ReaderEPUB() : currentPage(0), totalPages(0) {}

ReaderEPUB::~ReaderEPUB() {
    close();
}

void ReaderEPUB::close() {
    pages.clear();
    currentPage = 0;
    totalPages = 0;
    filename = "";
}

bool ReaderEPUB::loadFile(const std::string& path, vita2d_pgf* font) {
    close();

    size_t lastSlash = path.find_last_of("/\\");
    filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;

    parseEpubStructure(path, font);
    return !pages.empty();
}

void ReaderEPUB::parseEpubStructure(const std::string& path, vita2d_pgf* font) {
    // Parser de demonstração com quebra de capítulos
    pages.push_back("Capítulo 1\n\nEste é o início do livro em formato EPUB carregado pelo BilingualReaderVita.\n\nO sistema renderiza texto com aceleração de hardware pela GPU do console.");
    pages.push_back("Capítulo 2\n\nSegunda página do livro EPUB.\n\nPronto para suporte a múltiplos idiomas e recursos adicionais nas próximas fases.");
    totalPages = static_cast<int>(pages.size());
    currentPage = 0;
}

void ReaderEPUB::nextPage() {
    if (currentPage < totalPages - 1) {
        currentPage++;
    }
}

void ReaderEPUB::prevPage() {
    if (currentPage > 0) {
        currentPage--;
    }
}

void ReaderEPUB::render(vita2d_pgf* font) {
    // Fundo elegante para leitura de livro
    vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(20, 22, 31, 255));

    // Barra superior
    vita2d_draw_rectangle(0, 0, 960, 48, UITheme::TopBar);
    vita2d_draw_line(0, 48, 960, 48, RGBA8(42, 48, 70, 255));

    if (font) {
        vita2d_pgf_draw_text(font, 32, 32, UITheme::BadgeEPUB, 1.0f, filename.c_str());

        char pageInfo[64];
        snprintf(pageInfo, sizeof(pageInfo), "Pág %d / %d", currentPage + 1, totalPages > 0 ? totalPages : 1);
        vita2d_pgf_draw_text(font, 820, 32, UITheme::TextSecondary, 0.9f, pageInfo);

        if (!pages.empty() && currentPage < (int)pages.size()) {
            std::istringstream stream(pages[currentPage]);
            std::string line;
            int yPos = 90;
            while (std::getline(stream, line)) {
                vita2d_pgf_draw_text(font, 48, yPos, UITheme::TextPrimary, 1.0f, line.c_str());
                yPos += 28;
            }
        }
    }

    UIComponents::drawFooter("D-Pad Esq/Dir: Páginas | O: Voltar");
}
