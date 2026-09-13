#include <vita2d.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/ime_dialog.h>
#include <vector>
#include <string>
#include <algorithm>

#include "ui_components.h"
#include "file_browser.h"
#include "reader_txt.h"
#include "reader_cbz.h"
#include "reader_epub.h"

enum class AppState {
    MENU,
    READ_TXT,
    READ_CBZ,
    READ_EPUB
};

enum class FocusArea {
    SEARCH,
    SORT,
    LAYOUT,
    CONTENT
};

// Variáveis e rotinas para o teclado nativo (SceImeDialog)
static int ime_active = 0;
static uint16_t ime_title_utf16[SCE_IME_DIALOG_MAX_TITLE_LENGTH];
static uint16_t ime_text_utf16[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
static uint16_t ime_initial_utf16[SCE_IME_DIALOG_MAX_TEXT_LENGTH];

static void utf8_to_utf16(const uint8_t *src, uint16_t *dst) {
    int i = 0;
    for (i = 0; src[i]; i++) {
        dst[i] = src[i];
    }
    dst[i] = 0;
}

static void utf16_to_utf8(const uint16_t *src, uint8_t *dst) {
    int i = 0;
    for (i = 0; src[i]; i++) {
        dst[i] = (uint8_t)src[i];
    }
    dst[i] = 0;
}

static void launch_ime(const char* title, const char* initial_text) {
    SceImeDialogParam param;
    sceImeDialogParamInit(&param);
    param.sdkVersion = 0x03570011;
    param.supportedLanguages = 0x0001FFFF;
    param.languagesForced = SCE_FALSE;
    param.type = SCE_IME_TYPE_DEFAULT;
    param.option = 0;

    utf8_to_utf16((const uint8_t*)title, ime_title_utf16);
    param.title = ime_title_utf16;

    utf8_to_utf16((const uint8_t*)initial_text, ime_initial_utf16);
    param.initialText = ime_initial_utf16;

    param.maxTextLength = 128;
    param.inputTextBuffer = ime_text_utf16;

    sceImeDialogInit(&param);
    ime_active = 1;
}

int main(int argc, char* argv[]) {
    vita2d_init();
    vita2d_set_clear_color(UITheme::Background);

    vita2d_pgf* pgf = vita2d_load_default_pgf();
    UIComponents::init(pgf);

    AppState currentState = AppState::MENU;
    FocusArea currentFocus = FocusArea::CONTENT;

    std::vector<LibraryItem> allItems = FileBrowser::scanLibrary();
    SortMode currentSort = SortMode::NAME;
    bool isGridView = false;
    std::string searchQuery = "";

    std::vector<LibraryItem> visibleItems = FileBrowser::filterItems(allItems, searchQuery);

    int selectedIndex = 0;
    int scrollOffset = 0;

    ReaderTXT readerTxt;
    ReaderCBZ readerCbz;
    ReaderEPUB readerEpub;

    SceCtrlData pad;
    SceCtrlData oldPad;

    while (true) {
        // Manipulação do teclado nativo do PS Vita
        if (ime_active) {
            SceCommonDialogStatus status = sceImeDialogGetStatus();
            if (status == SCE_COMMON_DIALOG_STATUS_FINISHED) {
                SceImeDialogResult result;
                sceImeDialogGetResult(&result);
                if (result.button == SCE_IME_DIALOG_BUTTON_ENTER) {
                    uint8_t res_utf8[128];
                    utf16_to_utf8(ime_text_utf16, res_utf8);
                    searchQuery = reinterpret_cast<char*>(res_utf8);
                    visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                    selectedIndex = 0;
                    scrollOffset = 0;
                }
                sceImeDialogTerm();
                ime_active = 0;
            }

            vita2d_start_drawing();
            vita2d_clear_screen();
            vita2d_common_dialog_update();
            vita2d_end_drawing();
            vita2d_swap_buffers();
            continue;
        }

        sceCtrlPeekBufferPositive(0, &pad, 1);
        unsigned int pressed = pad.buttons & ~oldPad.buttons;

        if (currentState == AppState::MENU) {
            const int gridCols = 3;
            const int listVisibleCount = 5;
            const int gridVisibleCount = 6; // 3 colunas x 2 linhas

            // NAVEGAÇÃO ENTRE ÁREAS DE FOCO
            if (currentFocus == FocusArea::CONTENT) {
                if (isGridView) {
                    // Modo Grade
                    if (pressed & SCE_CTRL_UP) {
                        if (selectedIndex >= gridCols) {
                            selectedIndex -= gridCols;
                            if (selectedIndex < scrollOffset) {
                                scrollOffset -= gridCols;
                            }
                        } else {
                            currentFocus = FocusArea::SEARCH;
                        }
                    }
                    if (pressed & SCE_CTRL_DOWN) {
                        if (selectedIndex + gridCols < static_cast<int>(visibleItems.size())) {
                            selectedIndex += gridCols;
                            if (selectedIndex >= scrollOffset + gridVisibleCount) {
                                scrollOffset += gridCols;
                            }
                        }
                    }
                    if (pressed & SCE_CTRL_LEFT) {
                        if (selectedIndex > 0 && (selectedIndex % gridCols != 0)) {
                            selectedIndex--;
                        }
                    }
                    if (pressed & SCE_CTRL_RIGHT) {
                        if (selectedIndex + 1 < static_cast<int>(visibleItems.size()) && ((selectedIndex + 1) % gridCols != 0)) {
                            selectedIndex++;
                        }
                    }
                } else {
                    // Modo Lista
                    if (pressed & SCE_CTRL_UP) {
                        if (selectedIndex > 0) {
                            selectedIndex--;
                            if (selectedIndex < scrollOffset) {
                                scrollOffset = selectedIndex;
                            }
                        } else {
                            currentFocus = FocusArea::SEARCH;
                        }
                    }
                    if (pressed & SCE_CTRL_DOWN) {
                        if (selectedIndex < static_cast<int>(visibleItems.size()) - 1) {
                            selectedIndex++;
                            if (selectedIndex >= scrollOffset + listVisibleCount) {
                                scrollOffset = selectedIndex - listVisibleCount + 1;
                            }
                        }
                    }
                }

                // Alternar Favorito com Quadrado
                if ((pressed & SCE_CTRL_SQUARE) && !visibleItems.empty()) {
                    auto& item = visibleItems[selectedIndex];
                    FileBrowser::toggleFavorite(item);
                    for (auto& orig : allItems) {
                        if (orig.filename == item.filename) {
                            orig.isFavorite = item.isFavorite;
                            break;
                        }
                    }
                    FileBrowser::saveMetadata(allItems);
                }

                // Abrir Arquivo com Cruz
                if ((pressed & SCE_CTRL_CROSS) && !visibleItems.empty()) {
                    auto& selected = visibleItems[selectedIndex];
                    FileBrowser::markAsRead(selected);
                    for (auto& orig : allItems) {
                        if (orig.filename == selected.filename) {
                            orig.lastReadTime = selected.lastReadTime;
                            break;
                        }
                    }
                    FileBrowser::saveMetadata(allItems);

                    if (selected.type == FileType::TXT) {
                        if (readerTxt.loadFile(selected.fullPath, pgf)) {
                            currentState = AppState::READ_TXT;
                        }
                    } else if (selected.type == FileType::CBZ) {
                        if (readerCbz.loadFile(selected.fullPath)) {
                            currentState = AppState::READ_CBZ;
                        }
                    } else if (selected.type == FileType::EPUB) {
                        if (readerEpub.loadFile(selected.fullPath, pgf)) {
                            currentState = AppState::READ_EPUB;
                        }
                    }
                }
            } else if (currentFocus == FocusArea::SEARCH) {
                if (pressed & SCE_CTRL_DOWN) currentFocus = FocusArea::CONTENT;
                if (pressed & SCE_CTRL_RIGHT) currentFocus = FocusArea::SORT;
                if (pressed & SCE_CTRL_CROSS) {
                    launch_ime("Pesquisar", searchQuery.c_str());
                }
                if (pressed & SCE_CTRL_SQUARE) {
                    searchQuery = "";
                    visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                    selectedIndex = 0;
                    scrollOffset = 0;
                }
            } else if (currentFocus == FocusArea::SORT) {
                if (pressed & SCE_CTRL_DOWN) currentFocus = FocusArea::CONTENT;
                if (pressed & SCE_CTRL_LEFT) currentFocus = FocusArea::SEARCH;
                if (pressed & SCE_CTRL_RIGHT) currentFocus = FocusArea::LAYOUT;
                if (pressed & SCE_CTRL_CROSS) {
                    int nextSort = (static_cast<int>(currentSort) + 1) % static_cast<int>(SortMode::COUNT);
                    currentSort = static_cast<SortMode>(nextSort);
                    FileBrowser::sortItems(allItems, currentSort);
                    visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                    selectedIndex = 0;
                    scrollOffset = 0;
                }
            } else if (currentFocus == FocusArea::LAYOUT) {
                if (pressed & SCE_CTRL_DOWN) currentFocus = FocusArea::CONTENT;
                if (pressed & SCE_CTRL_LEFT) currentFocus = FocusArea::SORT;
                if (pressed & SCE_CTRL_CROSS) {
                    isGridView = !isGridView;
                    selectedIndex = 0;
                    scrollOffset = 0;
                }
            }

            // Atualizar lista geral com Triângulo
            if (pressed & SCE_CTRL_TRIANGLE) {
                allItems = FileBrowser::scanLibrary();
                FileBrowser::sortItems(allItems, currentSort);
                visibleItems = FileBrowser::filterItems(allItems, searchQuery);
                selectedIndex = 0;
                scrollOffset = 0;
            }

            // RENDERIZAÇÃO DO MENU
            vita2d_start_drawing();
            vita2d_clear_screen();

            UIComponents::drawTopBar(
                searchQuery,
                ime_active != 0,
                currentSort,
                isGridView,
                currentFocus == FocusArea::SEARCH,
                currentFocus == FocusArea::SORT,
                currentFocus == FocusArea::LAYOUT
            );

            if (visibleItems.empty()) {
                UIComponents::drawRoundedBox(180, 200, 600, 130, 12.0f, UITheme::Surface);
                if (pgf) {
                    vita2d_pgf_draw_text(pgf, 210, 250, UITheme::TextPrimary, 1.1f, "Nenhum arquivo correspondente!");
                    vita2d_pgf_draw_text(pgf, 210, 285, UITheme::TextSecondary, 0.85f, "Tente outra busca ou adicione arquivos em ux0:data/BilingualReaderVita/");
                }
            } else {
                if (!isGridView) {
                    // Renderização em Lista
                    float startY = 86.0f;
                    float cardHeight = 64.0f;
                    float cardSpacing = 14.0f;

                    int renderCount = std::min(static_cast<int>(visibleItems.size()) - scrollOffset, listVisibleCount);
                    for (int i = 0; i < renderCount; i++) {
                        int itemIdx = scrollOffset + i;
                        const auto& item = visibleItems[itemIdx];
                        bool isSelected = (currentFocus == FocusArea::CONTENT && itemIdx == selectedIndex);
                        float y = startY + i * (cardHeight + cardSpacing);

                        UIComponents::drawListCard(32.0f, y, 896.0f, cardHeight, isSelected, item);
                    }
                } else {
                    // Renderização em Grade
                    float startX = 32.0f;
                    float startY = 86.0f;
                    float cardW = 282.0f;
                    float cardH = 192.0f;
                    float gapX = 25.0f;
                    float gapY = 16.0f;

                    int renderCount = std::min(static_cast<int>(visibleItems.size()) - scrollOffset, gridVisibleCount);
                    for (int i = 0; i < renderCount; i++) {
                        int itemIdx = scrollOffset + i;
                        const auto& item = visibleItems[itemIdx];
                        bool isSelected = (currentFocus == FocusArea::CONTENT && itemIdx == selectedIndex);

                        int col = i % gridCols;
                        int row = i / gridCols;

                        float x = startX + col * (cardW + gapX);
                        float y = startY + row * (cardH + gapY);

                        UIComponents::drawGridCard(x, y, cardW, cardH, isSelected, item);
                    }
                }
            }

            UIComponents::drawFooter("D-Pad: Navegar | X: Abrir/Ação | []: Favorito/Limpar | /\\: Recarregar");
            vita2d_end_drawing();
            vita2d_swap_buffers();

        } else if (currentState == AppState::READ_TXT) {
            if ((pressed & SCE_CTRL_RIGHT) || (pressed & SCE_CTRL_RTRIGGER)) {
                readerTxt.nextPage();
            }
            if ((pressed & SCE_CTRL_LEFT) || (pressed & SCE_CTRL_LTRIGGER)) {
                readerTxt.prevPage();
            }
            if (pressed & SCE_CTRL_CIRCLE) {
                readerTxt.close();
                currentState = AppState::MENU;
            }

            vita2d_start_drawing();
            vita2d_clear_screen();
            readerTxt.render(pgf);
            vita2d_end_drawing();
            vita2d_swap_buffers();

        } else if (currentState == AppState::READ_CBZ) {
            if ((pressed & SCE_CTRL_RIGHT) || (pressed & SCE_CTRL_RTRIGGER)) {
                readerCbz.nextPage();
            }
            if ((pressed & SCE_CTRL_LEFT) || (pressed & SCE_CTRL_LTRIGGER)) {
                readerCbz.prevPage();
            }
            if (pressed & SCE_CTRL_CIRCLE) {
                readerCbz.close();
                currentState = AppState::MENU;
            }

            vita2d_start_drawing();
            vita2d_clear_screen();
            readerCbz.render(pgf);
            vita2d_end_drawing();
            vita2d_swap_buffers();

        } else if (currentState == AppState::READ_EPUB) {
            if ((pressed & SCE_CTRL_RIGHT) || (pressed & SCE_CTRL_RTRIGGER)) {
                readerEpub.nextPage();
            }
            if ((pressed & SCE_CTRL_LEFT) || (pressed & SCE_CTRL_LTRIGGER)) {
                readerEpub.prevPage();
            }
            if (pressed & SCE_CTRL_CIRCLE) {
                readerEpub.close();
                currentState = AppState::MENU;
            }

            vita2d_start_drawing();
            vita2d_clear_screen();
            readerEpub.render(pgf);
            vita2d_end_drawing();
            vita2d_swap_buffers();
        }

        oldPad = pad;
    }

    vita2d_free_pgf(pgf);
    vita2d_fini();
    sceKernelExitProcess(0);
    return 0;
}
