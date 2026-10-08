#ifndef PAGES_LINK_MANAGER_H
#define PAGES_LINK_MANAGER_H

#include "../models/linked_page.h"
#include "../parses/parse.h"
#include <string>
#include <vector>
#include <map>

class PagesLinkManager {
public:
    static PagesLinkManager& getInstance();

    // Inicialização e Carregamento de Arquivos
    bool loadPrimaryFile(const std::string& path);
    bool loadLinkedFile(const std::string& path);
    void clearLinkedFile();
    void close();

    // Informações dos Arquivos
    const std::string& getPrimaryPath() const { return m_primaryPath; }
    const std::string& getPrimaryName() const { return m_primaryName; }
    int getPrimaryPageCount() const { return m_primaryPages; }

    const std::string& getLinkedPath() const { return m_linkedPath; }
    const std::string& getLinkedName() const { return m_linkedName; }
    int getLinkedPageCount() const { return m_linkedPages; }

    bool isLinkedLoaded() const { return !m_linkedPath.empty() && m_linkedPages > 0; }
    Parse* getLinkedParse() const { return m_linkedParse; }

    // Listas de Páginas
    std::vector<LinkedPage>& getPagesLink() { return m_pagesLink; }
    const std::vector<LinkedPage>& getPagesLink() const { return m_pagesLink; }

    std::vector<LinkedPage>& getPagesNotLinked() { return m_pagesNotLinked; }
    const std::vector<LinkedPage>& getPagesNotLinked() const { return m_pagesNotLinked; }

    // Ações de Vínculo e Reordenação (Portadas do Android ViewModel)
    void onMove(int originIndex, int destinyIndex);
    void onMoveDualPage(bool originIsDual, int originIndex, bool destinyIsDual, int destinyIndex);
    void onNotLinked(int originIndex);
    void fromNotLinked(int notLinkedIndex, int destinyIndex);

    void reorderSimplePages(int startIndex = 0);
    void reorderDoublePages(bool isUseDualPageCalculate = false, int startIndex = 0);
    void autoReorderDoublePages(bool isClear = false);
    void reorderBySortPages();

    // Desfazer / Backup de Operações
    bool hasBackup() const { return !m_backupHistory.empty(); }
    void generateBackup();
    void returnBackup();
    void clearBackup();

    // Miniaturas e Gerenciamento de Memória
    vita2d_texture* getPrimaryThumbnail(int pageIndex);
    vita2d_texture* getLinkedThumbnail(int pageIndex);
    void purgeUnusedThumbnails(const std::vector<int>& keepPrimary, const std::vector<int>& keepLinked);
    void clearThumbnailCache();

    // Detecção Sob Demanda de Páginas Duplas
    bool isPrimaryDoublePage(int pageIndex);
    bool isLinkedDoublePage(int pageIndex);

    // Persistência em Disco
    bool save();
    bool loadSavedLink();
    static bool hasSavedLinkForFile(const std::string& primaryPath);
    static std::string getLinkFilePath(const std::string& primaryPath);

private:
    PagesLinkManager();
    ~PagesLinkManager();

    std::string m_primaryPath;
    std::string m_primaryName;
    int m_primaryPages = 0;
    Parse* m_primaryParse = nullptr;

    std::string m_linkedPath;
    std::string m_linkedName;
    int m_linkedPages = 0;
    Parse* m_linkedParse = nullptr;

    std::vector<LinkedPage> m_pagesLink;
    std::vector<LinkedPage> m_pagesNotLinked;

    // Cache de Texturas de Miniatura
    std::map<int, vita2d_texture*> m_primaryThumbs;
    std::map<int, vita2d_texture*> m_linkedThumbs;

    // Histórico de Backup para Undo
    struct BackupState {
        std::vector<LinkedPage> pagesLink;
        std::vector<LinkedPage> pagesNotLinked;
    };
    std::vector<BackupState> m_backupHistory;

    void addNotLinkedHelper(const LinkedPage& page);
};

#endif // PAGES_LINK_MANAGER_H
