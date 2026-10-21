#pragma once
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <vita2d.h>

class CoverManager {
public:
    static CoverManager& getInstance();

    // Inicializa o gerenciador e indexa os arquivos de capas existentes em disco
    void init();

    // Reescaneia o diretório de capas para sincronizar o índice em memória
    void refreshDiskIndex();

    // Retorna o caminho absoluto do arquivo de thumbnail em disco (em cache/cover)
    std::string getCoverPath(const std::string& bookPath);

    // Retorna apenas o nome do arquivo da capa
    std::string getCoverFileName(const std::string& bookPath);

    // Verifica em RAM (sem chamadas I/O no kernel) se a miniatura existe fisicamente no disco
    bool hasCoverOnDisk(const std::string& bookPath);

    // Obtém textura da miniatura carregada em VRAM para exibição (ou nullptr se não houver capa gerada)
    vita2d_texture* getCoverTexture(const std::string& bookPath);

    // Mantém o cache de texturas dentro do limite máximo de forma segura fora do loop de desenho
    void trimCache(size_t maxCount = 32);

    // Descarrega texturas não utilizadas (mantido por compatibilidade, não executa durante desenho ativo)
    void purgeUnused(const std::vector<std::string>& visibleBookPaths);

    // Libera 100% das texturas de capas em memória (ao sair para leitor, vincular páginas ou configurações)
    void clearAllTextures();

    // Gera a miniatura compacta da capa no disco se ainda não existir
    bool generateCover(const std::string& bookPath, bool force = false);

    // Exclui fisicamente todas as capas do cache em disco e limpa a memória (retorna quantidade excluída)
    int clearCoverCacheOnDisk();

    // Retorna o total de capas armazenadas atualmente em disco
    size_t getCoverCountOnDisk() const;

private:
    CoverManager();
    ~CoverManager();

    std::unordered_map<std::string, vita2d_texture*> m_textures;
    std::unordered_set<std::string> m_diskCoverFiles;
    std::list<std::string> m_lruList;
    std::unordered_map<std::string, std::list<std::string>::iterator> m_lruMap;
    bool m_indexInitialized = false;
};
