#ifndef LINKED_PAGE_H
#define LINKED_PAGE_H

#include <string>
#include <vector>
#include <vita2d.h>

constexpr int PAGE_EMPTY = -1;

struct LinkedPage {
    int idFile = 0;
    bool isNotLinked = false;

    // Manga Principal (ex: versão em japonês/inglês)
    int mangaPage = PAGE_EMPTY;
    int mangaPages = 0;
    std::string mangaPageName = "";
    std::string mangaPagePath = "";
    bool isMangaDualPage = false;

    // Arquivo de Vínculo (ex: versão em português) - Página Esquerda
    int fileLinkLeftPage = PAGE_EMPTY;
    int fileLinkLeftPages = 0;
    std::string fileLinkLeftPageName = "";
    std::string fileLinkLeftPagePath = "";
    bool isFileLeftDualPage = false;

    // Arquivo de Vínculo - Página Direita (quando página dupla vinculada a uma simples)
    int fileLinkRightPage = PAGE_EMPTY;
    std::string fileLinkRightPageName = "";
    std::string fileLinkRightPagePath = "";
    bool isFileRightDualPage = false;

    bool isDualImage = false;

    // Texturas de miniaturas (carregadas sob demanda)
    vita2d_texture* thumbManga = nullptr;
    vita2d_texture* thumbLeft = nullptr;
    vita2d_texture* thumbRight = nullptr;

    void merge(const LinkedPage& another) {
        this->fileLinkLeftPage = another.fileLinkLeftPage;
        this->fileLinkLeftPages = another.fileLinkLeftPages;
        this->fileLinkLeftPageName = another.fileLinkLeftPageName;
        this->fileLinkLeftPagePath = another.fileLinkLeftPagePath;
        this->fileLinkRightPage = another.fileLinkRightPage;
        this->fileLinkRightPageName = another.fileLinkRightPageName;
        this->fileLinkRightPagePath = another.fileLinkRightPagePath;
        this->thumbLeft = another.thumbLeft;
        this->thumbRight = another.thumbRight;
        this->isNotLinked = another.isNotLinked;
        this->isDualImage = another.isDualImage;
        this->isFileLeftDualPage = another.isFileLeftDualPage;
        this->isFileRightDualPage = another.isFileRightDualPage;
    }

    void addLeftPageLink(const LinkedPage& another) {
        addLeftPageLink(
            another.fileLinkLeftPage,
            another.fileLinkLeftPages,
            another.fileLinkLeftPageName,
            another.fileLinkLeftPagePath,
            another.isFileLeftDualPage,
            another.thumbLeft
        );
    }

    void addLeftFromRightPageLink(const LinkedPage& another) {
        addLeftPageLink(
            another.fileLinkRightPage,
            another.fileLinkLeftPages,
            another.fileLinkRightPageName,
            another.fileLinkRightPagePath,
            another.isFileRightDualPage,
            another.thumbRight
        );
    }

    void addLeftPageLink(int page, int pages, const std::string& pageName, const std::string& pagePath, bool isDual, vita2d_texture* thumb) {
        this->fileLinkLeftPage = page;
        this->fileLinkLeftPages = pages;
        this->fileLinkLeftPageName = pageName;
        this->fileLinkLeftPagePath = pagePath;
        this->thumbLeft = thumb;
        this->isFileLeftDualPage = isDual;
    }

    void addRightFromLeftPageLink(const LinkedPage& another) {
        addRightPageLink(
            another.fileLinkLeftPage,
            another.fileLinkLeftPageName,
            another.fileLinkLeftPagePath,
            another.isFileLeftDualPage,
            another.thumbLeft
        );
    }

    void addRightPageLink(const LinkedPage& another) {
        addRightPageLink(
            another.fileLinkRightPage,
            another.fileLinkRightPageName,
            another.fileLinkRightPagePath,
            another.isFileRightDualPage,
            another.thumbRight
        );
    }

    void addRightPageLink(int page, const std::string& pageName, const std::string& pagePath, bool isDual, vita2d_texture* thumb) {
        if (this->fileLinkLeftPage == PAGE_EMPTY) {
            this->fileLinkLeftPage = page;
            this->fileLinkLeftPageName = pageName;
            this->fileLinkLeftPagePath = pagePath;
            this->thumbLeft = thumb;
            this->isFileLeftDualPage = isDual;
        } else {
            this->fileLinkRightPage = page;
            this->fileLinkRightPageName = pageName;
            this->fileLinkRightPagePath = pagePath;
            this->thumbRight = thumb;
            this->isFileRightDualPage = isDual;
        }
        this->isDualImage = (this->fileLinkRightPage != PAGE_EMPTY);
    }

    void movePageLinkRightToLeft() {
        this->fileLinkLeftPage = this->fileLinkRightPage;
        this->fileLinkLeftPageName = this->fileLinkRightPageName;
        this->fileLinkLeftPagePath = this->fileLinkRightPagePath;
        this->thumbLeft = this->thumbRight;
        this->isFileLeftDualPage = this->isFileRightDualPage;
        clearRightPageLink();
    }

    void clearPageLink() {
        this->fileLinkLeftPage = PAGE_EMPTY;
        this->fileLinkLeftPages = 0;
        this->fileLinkLeftPageName = "";
        this->fileLinkLeftPagePath = "";
        this->fileLinkRightPage = PAGE_EMPTY;
        this->fileLinkRightPageName = "";
        this->fileLinkRightPagePath = "";
        this->thumbLeft = nullptr;
        this->thumbRight = nullptr;
        this->isFileLeftDualPage = false;
        this->isFileRightDualPage = false;
        this->isNotLinked = false;
        this->isDualImage = false;
    }

    bool clearLeftPageLink(bool canMove = false) {
        if (canMove && this->fileLinkRightPage != PAGE_EMPTY) {
            this->fileLinkLeftPage = this->fileLinkRightPage;
            this->fileLinkLeftPageName = this->fileLinkRightPageName;
            this->fileLinkLeftPagePath = this->fileLinkRightPagePath;
            this->thumbLeft = this->thumbRight;
            this->isFileLeftDualPage = this->isFileRightDualPage;
            clearRightPageLink();
            return true;
        } else {
            this->fileLinkLeftPage = PAGE_EMPTY;
            this->fileLinkLeftPages = 0;
            this->fileLinkLeftPageName = "";
            this->fileLinkLeftPagePath = "";
            this->thumbLeft = nullptr;
            this->isFileLeftDualPage = false;
            return false;
        }
    }

    void clearRightPageLink() {
        this->fileLinkRightPage = PAGE_EMPTY;
        this->fileLinkRightPageName = "";
        this->fileLinkRightPagePath = "";
        this->thumbRight = nullptr;
        this->isDualImage = false;
        this->isFileRightDualPage = false;
    }
};

struct LinkedFile {
    int id = 0;
    std::string primaryPath = "";
    std::string primaryName = "";
    int primaryPages = 0;

    std::string linkedPath = "";
    std::string linkedName = "";
    int linkedPages = 0;

    std::vector<LinkedPage> pagesLink;
    std::vector<LinkedPage> pagesNotLinked;

    void clear() {
        id = 0;
        primaryPath = "";
        primaryName = "";
        primaryPages = 0;
        linkedPath = "";
        linkedName = "";
        linkedPages = 0;
        pagesLink.clear();
        pagesNotLinked.clear();
    }
};

#endif // LINKED_PAGE_H
