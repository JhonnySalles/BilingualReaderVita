#include "pages_link_manager.h"
#include "../parses/parse_factory.h"
#include "../file_utils.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

PagesLinkManager& PagesLinkManager::getInstance() {
    static PagesLinkManager instance;
    return instance;
}

PagesLinkManager::PagesLinkManager() {}

PagesLinkManager::~PagesLinkManager() {
    close();
}

void PagesLinkManager::close() {
    clearThumbnailCache();
    clearBackup();

    if (m_primaryParse) {
        ParseFactory::destroy(m_primaryParse);
    }
    if (m_linkedParse) {
        ParseFactory::destroy(m_linkedParse);
    }

    m_primaryPath.clear();
    m_primaryName.clear();
    m_primaryPages = 0;

    m_linkedPath.clear();
    m_linkedName.clear();
    m_linkedPages = 0;

    m_pagesLink.clear();
    m_pagesNotLinked.clear();
}

void PagesLinkManager::clearThumbnailCache() {
    for (auto& pair : m_primaryThumbs) {
        if (pair.second) {
            vita2d_free_texture(pair.second);
        }
    }
    m_primaryThumbs.clear();

    for (auto& pair : m_linkedThumbs) {
        if (pair.second) {
            vita2d_free_texture(pair.second);
        }
    }
    m_linkedThumbs.clear();
}

void PagesLinkManager::purgeUnusedThumbnails(int currentCenterIndex, int windowSize) {
    int minIdx = currentCenterIndex - windowSize;
    int maxIdx = currentCenterIndex + windowSize;

    auto itPrim = m_primaryThumbs.begin();
    while (itPrim != m_primaryThumbs.end()) {
        if (itPrim->first < minIdx || itPrim->first > maxIdx) {
            if (itPrim->second) vita2d_free_texture(itPrim->second);
            itPrim = m_primaryThumbs.erase(itPrim);
        } else {
            ++itPrim;
        }
    }

    auto itLink = m_linkedThumbs.begin();
    while (itLink != m_linkedThumbs.end()) {
        if (itLink->first < minIdx || itLink->first > maxIdx) {
            if (itLink->second) vita2d_free_texture(itLink->second);
            itLink = m_linkedThumbs.erase(itLink);
        } else {
            ++itLink;
        }
    }
}

vita2d_texture* PagesLinkManager::getPrimaryThumbnail(int pageIndex) {
    if (pageIndex < 0 || pageIndex >= m_primaryPages || !m_primaryParse) return nullptr;

    auto it = m_primaryThumbs.find(pageIndex);
    if (it != m_primaryThumbs.end()) {
        return it->second;
    }

    vita2d_texture* tex = m_primaryParse->loadPageThumbnail(pageIndex, 100, 140);
    m_primaryThumbs[pageIndex] = tex;
    return tex;
}

vita2d_texture* PagesLinkManager::getLinkedThumbnail(int pageIndex) {
    if (pageIndex < 0 || pageIndex >= m_linkedPages || !m_linkedParse) return nullptr;

    auto it = m_linkedThumbs.find(pageIndex);
    if (it != m_linkedThumbs.end()) {
        return it->second;
    }

    vita2d_texture* tex = m_linkedParse->loadPageThumbnail(pageIndex, 100, 140);
    m_linkedThumbs[pageIndex] = tex;
    return tex;
}

bool PagesLinkManager::loadPrimaryFile(const std::string& path) {
    close();

    m_primaryParse = ParseFactory::create(path);
    if (!m_primaryParse) return false;

    m_primaryPath = path;
    m_primaryName = FileUtils::getFileName(path);
    m_primaryPages = static_cast<int>(m_primaryParse->getPageCount());

    m_pagesLink.resize(m_primaryPages);
    for (int i = 0; i < m_primaryPages; i++) {
        m_pagesLink[i].mangaPage = i;
        m_pagesLink[i].mangaPages = m_primaryPages;
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", i + 1);
        m_pagesLink[i].mangaPageName = buf;
        m_pagesLink[i].isMangaDualPage = m_primaryParse->isDoublePage(i);
        m_pagesLink[i].clearPageLink();
    }

    // Tenta carregar vínculo salvo anteriormente se existir
    if (hasSavedLinkForFile(path)) {
        loadSavedLink();
    }

    return true;
}

bool PagesLinkManager::loadLinkedFile(const std::string& path) {
    if (m_linkedParse) {
        ParseFactory::destroy(m_linkedParse);
    }
    for (auto& pair : m_linkedThumbs) {
        if (pair.second) vita2d_free_texture(pair.second);
    }
    m_linkedThumbs.clear();
    m_pagesNotLinked.clear();

    m_linkedParse = ParseFactory::create(path);
    if (!m_linkedParse) return false;

    m_linkedPath = path;
    m_linkedName = FileUtils::getFileName(path);
    m_linkedPages = static_cast<int>(m_linkedParse->getPageCount());

    generateBackup();

    for (int i = 0; i < m_primaryPages; i++) {
        m_pagesLink[i].clearPageLink();
    }

    // Mapeamento inicial 1 para 1
    int minPages = std::min(m_primaryPages, m_linkedPages);
    for (int i = 0; i < minPages; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", i + 1);
        m_pagesLink[i].fileLinkLeftPage = i;
        m_pagesLink[i].fileLinkLeftPages = m_linkedPages;
        m_pagesLink[i].fileLinkLeftPageName = buf;
        m_pagesLink[i].isFileLeftDualPage = m_linkedParse->isDoublePage(i);
        m_pagesLink[i].isDualImage = false;
    }

    // Páginas excedentes vão para a lista de Não Vinculadas
    for (int i = minPages; i < m_linkedPages; i++) {
        LinkedPage p;
        p.isNotLinked = true;
        p.fileLinkLeftPage = i;
        p.fileLinkLeftPages = m_linkedPages;
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", i + 1);
        p.fileLinkLeftPageName = buf;
        p.isFileLeftDualPage = m_linkedParse->isDoublePage(i);
        m_pagesNotLinked.push_back(p);
    }

    return true;
}

void PagesLinkManager::clearLinkedFile() {
    generateBackup();

    for (auto& pair : m_linkedThumbs) {
        if (pair.second) vita2d_free_texture(pair.second);
    }
    m_linkedThumbs.clear();

    if (m_linkedParse) {
        ParseFactory::destroy(m_linkedParse);
    }
    m_linkedPath.clear();
    m_linkedName.clear();
    m_linkedPages = 0;

    for (auto& p : m_pagesLink) {
        p.clearPageLink();
    }
    m_pagesNotLinked.clear();
}

void PagesLinkManager::addNotLinkedHelper(const LinkedPage& page) {
    if (page.fileLinkLeftPage != PAGE_EMPTY) {
        LinkedPage p;
        p.isNotLinked = true;
        p.fileLinkLeftPage = page.fileLinkLeftPage;
        p.fileLinkLeftPages = page.fileLinkLeftPages;
        p.fileLinkLeftPageName = page.fileLinkLeftPageName;
        p.fileLinkLeftPagePath = page.fileLinkLeftPagePath;
        p.isFileLeftDualPage = page.isFileLeftDualPage;
        m_pagesNotLinked.push_back(p);
    }
    if (page.fileLinkRightPage != PAGE_EMPTY) {
        LinkedPage p;
        p.isNotLinked = true;
        p.fileLinkLeftPage = page.fileLinkRightPage;
        p.fileLinkLeftPages = page.fileLinkLeftPages;
        p.fileLinkLeftPageName = page.fileLinkRightPageName;
        p.fileLinkLeftPagePath = page.fileLinkRightPagePath;
        p.isFileLeftDualPage = page.isFileRightDualPage;
        m_pagesNotLinked.push_back(p);
    }
}

void PagesLinkManager::onMove(int originIndex, int destinyIndex) {
    if (originIndex < 0 || originIndex >= (int)m_pagesLink.size() ||
        destinyIndex < 0 || destinyIndex >= (int)m_pagesLink.size() ||
        originIndex == destinyIndex) {
        return;
    }

    generateBackup();

    LinkedPage origin = m_pagesLink[originIndex];
    int differ = destinyIndex - originIndex;
    int totalSize = (int)m_pagesLink.size();

    if (originIndex > destinyIndex) {
        int limit = totalSize - 1;

        for (int i = limit; i >= originIndex; i--) {
            if (m_pagesLink[i].fileLinkLeftPage == PAGE_EMPTY) {
                limit = i;
            }
        }

        for (int i = destinyIndex; i < originIndex; i++) {
            addNotLinkedHelper(m_pagesLink[i]);
        }

        differ *= -1;
        for (int i = destinyIndex; i <= limit; i++) {
            if (i == destinyIndex) {
                m_pagesLink[i].addLeftPageLink(origin);
                origin.clearLeftPageLink();
            } else if ((i + differ) > limit) {
                continue;
            } else {
                m_pagesLink[i].addLeftPageLink(m_pagesLink[i + differ]);
                m_pagesLink[i + differ].clearLeftPageLink();
            }
        }

        for (int i = destinyIndex; i <= limit; i++) {
            if (m_pagesLink[i].isDualImage && m_pagesLink[i].fileLinkLeftPage == PAGE_EMPTY &&
                m_pagesLink[i].fileLinkRightPage != PAGE_EMPTY) {
                m_pagesLink[i].movePageLinkRightToLeft();
            }
        }
    } else {
        int limit = totalSize - 1;
        int spacesFree = 0;

        for (int i = originIndex; i <= limit; i++) {
            if (m_pagesLink[i].fileLinkLeftPage == PAGE_EMPTY) {
                spacesFree++;
            }
        }

        if (differ > spacesFree) {
            int startAdd = std::max(0, limit - differ + 1);
            for (int i = limit; i >= startAdd; i--) {
                addNotLinkedHelper(m_pagesLink[i]);
            }

            for (int i = limit; i >= originIndex; i--) {
                if (i < destinyIndex) {
                    m_pagesLink[i].clearLeftPageLink();
                } else if (i - differ >= 0) {
                    m_pagesLink[i].addLeftPageLink(m_pagesLink[i - differ]);
                }
            }
        } else {
            int spaceUsed = 0;
            for (int i = originIndex; i <= limit; i++) {
                if (m_pagesLink[i].fileLinkLeftPage == PAGE_EMPTY) {
                    spaceUsed++;
                    if (spaceUsed >= differ) {
                        limit = i;
                        break;
                    }
                }
            }

            spaceUsed = 0;
            for (int i = limit; i >= originIndex; i--) {
                if (i < destinyIndex) {
                    m_pagesLink[i].clearLeftPageLink(true);
                } else {
                    int idx = i - (1 + spaceUsed);
                    if (idx >= 0 && m_pagesLink[idx].fileLinkLeftPage == PAGE_EMPTY) {
                        do {
                            spaceUsed++;
                            idx = i - (1 + spaceUsed);
                        } while (idx >= 0 && m_pagesLink[idx].fileLinkLeftPage == PAGE_EMPTY);
                    }
                    if (idx >= 0) {
                        m_pagesLink[i].addLeftPageLink(m_pagesLink[idx]);
                    }
                }
            }
        }
    }
}

void PagesLinkManager::onMoveDualPage(bool originIsDual, int originIndex, bool destinyIsDual, int destinyIndex) {
    if (originIndex < 0 || originIndex >= (int)m_pagesLink.size() ||
        destinyIndex < 0 || destinyIndex >= (int)m_pagesLink.size()) return;

    generateBackup();

    LinkedPage& origin = m_pagesLink[originIndex];
    LinkedPage& destiny = m_pagesLink[destinyIndex];

    if (!originIsDual && destinyIsDual) {
        destiny.addRightFromLeftPageLink(origin);
        origin.clearLeftPageLink(true);
    } else if (originIsDual && !destinyIsDual) {
        destiny.addLeftFromRightPageLink(origin);
        origin.clearRightPageLink();
    } else if (originIsDual && destinyIsDual) {
        destiny.addRightPageLink(origin.fileLinkRightPage, origin.fileLinkRightPageName, origin.fileLinkRightPagePath, origin.isFileRightDualPage, origin.thumbRight);
        origin.clearRightPageLink();
    }
}

void PagesLinkManager::onNotLinked(int originIndex) {
    if (originIndex < 0 || originIndex >= (int)m_pagesLink.size()) return;

    generateBackup();
    LinkedPage& origin = m_pagesLink[originIndex];

    if (origin.fileLinkLeftPage != PAGE_EMPTY) {
        LinkedPage p;
        p.isNotLinked = true;
        p.fileLinkLeftPage = origin.fileLinkLeftPage;
        p.fileLinkLeftPages = origin.fileLinkLeftPages;
        p.fileLinkLeftPageName = origin.fileLinkLeftPageName;
        p.fileLinkLeftPagePath = origin.fileLinkLeftPagePath;
        p.isFileLeftDualPage = origin.isFileLeftDualPage;
        m_pagesNotLinked.push_back(p);
    }

    if (origin.isDualImage) {
        origin.movePageLinkRightToLeft();
    } else {
        origin.clearPageLink();
    }
}

void PagesLinkManager::fromNotLinked(int notLinkedIndex, int destinyIndex) {
    if (notLinkedIndex < 0 || notLinkedIndex >= (int)m_pagesNotLinked.size() ||
        destinyIndex < 0 || destinyIndex >= (int)m_pagesLink.size()) return;

    generateBackup();
    LinkedPage item = m_pagesNotLinked[notLinkedIndex];
    m_pagesNotLinked.erase(m_pagesNotLinked.begin() + notLinkedIndex);

    int totalSize = (int)m_pagesLink.size();
    if (m_pagesLink[destinyIndex].fileLinkLeftPage == PAGE_EMPTY) {
        m_pagesLink[destinyIndex].addLeftPageLink(item);
    } else {
        addNotLinkedHelper(m_pagesLink[totalSize - 1]);
        for (int i = totalSize - 1; i > destinyIndex; i--) {
            m_pagesLink[i].addLeftPageLink(m_pagesLink[i - 1]);
        }
        m_pagesLink[destinyIndex].addLeftPageLink(item);
    }
}

void PagesLinkManager::reorderSimplePages(int startIndex) {
    if (m_pagesLink.empty()) return;
    generateBackup();

    std::vector<LinkedPage> allLinkedItems;
    for (size_t i = 0; i < m_pagesLink.size(); i++) {
        if ((int)i >= startIndex) {
            if (m_pagesLink[i].fileLinkLeftPage != PAGE_EMPTY) {
                LinkedPage p = m_pagesLink[i];
                p.clearRightPageLink();
                allLinkedItems.push_back(p);
            }
            if (m_pagesLink[i].fileLinkRightPage != PAGE_EMPTY) {
                LinkedPage p;
                p.fileLinkLeftPage = m_pagesLink[i].fileLinkRightPage;
                p.fileLinkLeftPages = m_pagesLink[i].fileLinkLeftPages;
                p.fileLinkLeftPageName = m_pagesLink[i].fileLinkRightPageName;
                p.fileLinkLeftPagePath = m_pagesLink[i].fileLinkRightPagePath;
                p.isFileLeftDualPage = m_pagesLink[i].isFileRightDualPage;
                allLinkedItems.push_back(p);
            }
            m_pagesLink[i].clearPageLink();
        }
    }

    size_t curItem = 0;
    for (size_t i = startIndex; i < m_pagesLink.size() && curItem < allLinkedItems.size(); i++, curItem++) {
        m_pagesLink[i].addLeftPageLink(allLinkedItems[curItem]);
    }

    while (curItem < allLinkedItems.size()) {
        addNotLinkedHelper(allLinkedItems[curItem]);
        curItem++;
    }
}

void PagesLinkManager::reorderDoublePages(bool isUseDualPageCalculate, int startIndex) {
    if (m_pagesLink.empty()) return;
    generateBackup();

    for (size_t i = startIndex; i < m_pagesLink.size() - 1; i++) {
        if (m_pagesLink[i].isDualImage) continue;

        if (m_pagesLink[i].fileLinkLeftPage != PAGE_EMPTY &&
            m_pagesLink[i + 1].fileLinkLeftPage != PAGE_EMPTY &&
            !m_pagesLink[i + 1].isDualImage) {
            
            m_pagesLink[i].addRightFromLeftPageLink(m_pagesLink[i + 1]);
            m_pagesLink[i + 1].clearPageLink();

            // Shift subseqüente
            for (size_t k = i + 1; k < m_pagesLink.size() - 1; k++) {
                m_pagesLink[k].addLeftPageLink(m_pagesLink[k + 1]);
                m_pagesLink[k + 1].clearLeftPageLink();
            }
        }
    }
}

void PagesLinkManager::autoReorderDoublePages(bool isClear) {
    if (m_pagesLink.empty()) return;

    if (isClear) {
        reorderSimplePages(0);
    } else {
        generateBackup();
    }

    int lastIndex = (int)m_pagesLink.size() - 1;
    for (int i = 0; i < lastIndex; i++) {
        if (m_pagesLink[i].fileLinkLeftPage == PAGE_EMPTY) continue;

        if (m_pagesLink[i].isMangaDualPage) {
            if (i + 1 < (int)m_pagesLink.size()) {
                LinkedPage& nextPage = m_pagesLink[i + 1];
                if (nextPage.fileLinkLeftPage != PAGE_EMPTY && !nextPage.isFileLeftDualPage) {
                    m_pagesLink[i].addRightFromLeftPageLink(nextPage);
                    nextPage.clearPageLink();

                    for (int k = i + 1; k < lastIndex; k++) {
                        m_pagesLink[k].addLeftPageLink(m_pagesLink[k + 1]);
                        m_pagesLink[k + 1].clearLeftPageLink();
                    }
                }
            }
        }
    }
}

void PagesLinkManager::reorderBySortPages() {
    if (m_pagesLink.empty()) return;
    generateBackup();

    std::vector<LinkedPage> pool;
    for (auto& p : m_pagesLink) {
        if (p.fileLinkLeftPage != PAGE_EMPTY) {
            LinkedPage item;
            item.fileLinkLeftPage = p.fileLinkLeftPage;
            item.fileLinkLeftPages = p.fileLinkLeftPages;
            item.fileLinkLeftPageName = p.fileLinkLeftPageName;
            item.fileLinkLeftPagePath = p.fileLinkLeftPagePath;
            item.isFileLeftDualPage = p.isFileLeftDualPage;
            pool.push_back(item);
        }
        if (p.fileLinkRightPage != PAGE_EMPTY) {
            LinkedPage item;
            item.fileLinkLeftPage = p.fileLinkRightPage;
            item.fileLinkLeftPages = p.fileLinkLeftPages;
            item.fileLinkLeftPageName = p.fileLinkRightPageName;
            item.fileLinkLeftPagePath = p.fileLinkRightPagePath;
            item.isFileLeftDualPage = p.isFileRightDualPage;
            pool.push_back(item);
        }
        p.clearPageLink();
    }
    for (auto& p : m_pagesNotLinked) {
        if (p.fileLinkLeftPage != PAGE_EMPTY) {
            pool.push_back(p);
        }
    }
    m_pagesNotLinked.clear();

    std::sort(pool.begin(), pool.end(), [](const LinkedPage& a, const LinkedPage& b) {
        return a.fileLinkLeftPage < b.fileLinkLeftPage;
    });

    size_t cur = 0;
    for (size_t i = 0; i < m_pagesLink.size() && cur < pool.size(); i++, cur++) {
        m_pagesLink[i].addLeftPageLink(pool[cur]);
    }
    while (cur < pool.size()) {
        m_pagesNotLinked.push_back(pool[cur]);
        cur++;
    }
}

void PagesLinkManager::generateBackup() {
    BackupState state;
    state.pagesLink = m_pagesLink;
    state.pagesNotLinked = m_pagesNotLinked;

    if (m_backupHistory.size() >= 12) {
        m_backupHistory.erase(m_backupHistory.begin());
    }
    m_backupHistory.push_back(state);
}

void PagesLinkManager::returnBackup() {
    if (m_backupHistory.empty()) return;

    BackupState state = m_backupHistory.back();
    m_backupHistory.pop_back();

    m_pagesLink = state.pagesLink;
    m_pagesNotLinked = state.pagesNotLinked;
}

void PagesLinkManager::clearBackup() {
    m_backupHistory.clear();
}

std::string PagesLinkManager::getLinkFilePath(const std::string& primaryPath) {
    std::string base = "ux0:data/bilingual_reader/links/";
    std::string name = FileUtils::getFileNameWithoutExtension(primaryPath);
    return base + name + ".link.txt";
}

bool PagesLinkManager::hasSavedLinkForFile(const std::string& primaryPath) {
    if (primaryPath.empty()) return false;
    std::string linkPath = getLinkFilePath(primaryPath);
    return FileUtils::fileExists(linkPath);
}

bool PagesLinkManager::save() {
    if (m_primaryPath.empty()) return false;

    FileUtils::makeDirRecursive("ux0:data/bilingual_reader/links");
    std::string path = getLinkFilePath(m_primaryPath);

    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << "[Header]\n";
    file << "primary_path=" << m_primaryPath << "\n";
    file << "primary_pages=" << m_primaryPages << "\n";
    file << "linked_path=" << m_linkedPath << "\n";
    file << "linked_pages=" << m_linkedPages << "\n";

    file << "\n[LinkedPages]\n";
    for (const auto& p : m_pagesLink) {
        file << p.mangaPage << "|"
             << p.fileLinkLeftPage << "|" << p.fileLinkLeftPageName << "|"
             << p.fileLinkRightPage << "|" << p.fileLinkRightPageName << "|"
             << (p.isDualImage ? "1" : "0") << "|"
             << (p.isMangaDualPage ? "1" : "0") << "|"
             << (p.isFileLeftDualPage ? "1" : "0") << "|"
             << (p.isFileRightDualPage ? "1" : "0") << "\n";
    }

    file << "\n[UnlinkedPages]\n";
    for (const auto& p : m_pagesNotLinked) {
        file << p.fileLinkLeftPage << "|" << p.fileLinkLeftPageName << "|"
             << (p.isFileLeftDualPage ? "1" : "0") << "\n";
    }

    file.close();
    return true;
}

bool PagesLinkManager::loadSavedLink() {
    if (m_primaryPath.empty()) return false;

    std::string path = getLinkFilePath(m_primaryPath);
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string line;
    std::string currentSection;
    std::string savedLinkedPath;
    int savedLinkedPages = 0;

    std::vector<LinkedPage> loadedPages;
    std::vector<LinkedPage> loadedNotLinked;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();

        if (line.front() == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }

        if (currentSection == "Header") {
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = line.substr(0, eqPos);
                std::string val = line.substr(eqPos + 1);
                if (key == "linked_path") savedLinkedPath = val;
                else if (key == "linked_pages") savedLinkedPages = std::stoi(val);
            }
        } else if (currentSection == "LinkedPages") {
            std::stringstream ss(line);
            std::string token;
            std::vector<std::string> tokens;
            while (std::getline(ss, token, '|')) {
                tokens.push_back(token);
            }
            if (tokens.size() >= 9) {
                LinkedPage p;
                p.mangaPage = std::stoi(tokens[0]);
                p.mangaPages = m_primaryPages;
                char buf[32];
                snprintf(buf, sizeof(buf), "%d", p.mangaPage + 1);
                p.mangaPageName = buf;

                p.fileLinkLeftPage = std::stoi(tokens[1]);
                p.fileLinkLeftPageName = tokens[2];
                p.fileLinkRightPage = std::stoi(tokens[3]);
                p.fileLinkRightPageName = tokens[4];
                p.isDualImage = (tokens[5] == "1");
                p.isMangaDualPage = (tokens[6] == "1");
                p.isFileLeftDualPage = (tokens[7] == "1");
                p.isFileRightDualPage = (tokens[8] == "1");
                loadedPages.push_back(p);
            }
        } else if (currentSection == "UnlinkedPages") {
            std::stringstream ss(line);
            std::string token;
            std::vector<std::string> tokens;
            while (std::getline(ss, token, '|')) {
                tokens.push_back(token);
            }
            if (tokens.size() >= 3) {
                LinkedPage p;
                p.isNotLinked = true;
                p.fileLinkLeftPage = std::stoi(tokens[0]);
                p.fileLinkLeftPageName = tokens[1];
                p.isFileLeftDualPage = (tokens[2] == "1");
                loadedNotLinked.push_back(p);
            }
        }
    }
    file.close();

    if (!savedLinkedPath.empty() && FileUtils::fileExists(savedLinkedPath)) {
        if (m_linkedParse) ParseFactory::destroy(m_linkedParse);
        m_linkedParse = ParseFactory::create(savedLinkedPath);
        m_linkedPath = savedLinkedPath;
        m_linkedName = FileUtils::getFileName(savedLinkedPath);
        m_linkedPages = savedLinkedPages;
    }

    if (!loadedPages.empty()) {
        m_pagesLink = loadedPages;
        m_pagesNotLinked = loadedNotLinked;
        return true;
    }

    return false;
}
