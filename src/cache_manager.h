#ifndef CACHE_MANAGER_H
#define CACHE_MANAGER_H

#include <string>

class CacheManager {
public:
    static CacheManager& getInstance();

    // Inicializa a árvore de diretórios (library, cache/cover, cache/reader) e limpa resíduos
    void init();

    // Retorna o caminho base da biblioteca
    std::string getLibraryPath() const;

    // Retorna o caminho base de cache
    std::string getCacheBasePath() const;

    // Retorna o caminho do diretório de capas
    std::string getCoverCachePath() const;

    // Cria e retorna uma pasta única randômica para a sessão de leitura ativa
    std::string createReaderSessionPath();

    // Remove a pasta de sessão de leitura após o término
    void cleanupReaderSession(const std::string& sessionPath);

    // Limpa todo o cache de leitura temporário
    void cleanupAllReaderSessions();

    // Retorna o caminho esperado da capa para um arquivo de livro/manga
    std::string getCoverFilePath(const std::string& bookPath);

private:
    CacheManager();
    ~CacheManager();

    std::string m_baseDir;
    std::string m_libraryDir;
    std::string m_cacheDir;
    std::string m_coverDir;
    std::string m_readerDir;
};

#endif // CACHE_MANAGER_H
