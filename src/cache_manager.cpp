#include "cache_manager.h"
#include "file_utils.h"
#include <functional>

CacheManager& CacheManager::getInstance() {
    static CacheManager instance;
    return instance;
}

CacheManager::CacheManager() {
#ifdef __vita__
    m_baseDir = "ux0:data/BilingualReaderVita";
#else
    m_baseDir = "./BilingualReaderVita";
#endif
    m_libraryDir = m_baseDir + "/library";
    m_cacheDir = m_baseDir + "/cache";
    m_coverDir = m_cacheDir + "/cover";
    m_readerDir = m_cacheDir + "/reader";
}

CacheManager::~CacheManager() {
}

void CacheManager::init() {
    FileUtils::makeDirRecursive(m_baseDir);
    FileUtils::makeDirRecursive(m_libraryDir);
    FileUtils::makeDirRecursive(m_cacheDir);
    FileUtils::makeDirRecursive(m_coverDir);
    FileUtils::makeDirRecursive(m_readerDir);

    // Limpa resíduos de sessões anteriores que possam ter ficado após fechamento inesperado
    cleanupAllReaderSessions();
}

std::string CacheManager::getLibraryPath() const {
    return m_libraryDir;
}

std::string CacheManager::getCacheBasePath() const {
    return m_cacheDir;
}

std::string CacheManager::getCoverCachePath() const {
    return m_coverDir;
}

std::string CacheManager::createReaderSessionPath() {
    std::string randomSession = FileUtils::generateRandomId(10);
    std::string sessionPath = m_readerDir + "/sess_" + randomSession;
    FileUtils::makeDirRecursive(sessionPath);
    return sessionPath;
}

void CacheManager::cleanupReaderSession(const std::string& sessionPath) {
    if (!sessionPath.empty()) {
        FileUtils::removeDirRecursive(sessionPath);
    }
}

void CacheManager::cleanupAllReaderSessions() {
    FileUtils::removeDirRecursive(m_readerDir);
    FileUtils::makeDirRecursive(m_readerDir);
}

std::string CacheManager::getCoverFilePath(const std::string& bookPath) {
    std::string fileName = FileUtils::getFileName(bookPath);
    std::hash<std::string> hasher;
    size_t hashValue = hasher(bookPath);
    return m_coverDir + "/" + std::to_string(hashValue) + "_" + fileName + ".png";
}
