#ifndef PARSE_H
#define PARSE_H

#include <string>
#include <vector>
#include <vita2d.h>

class Parse {
public:
    virtual ~Parse() {}

    // Abre o arquivo compactado e indexa a lista de páginas
    virtual bool open(const std::string& path) = 0;

    // Fecha o arquivo e libera recursos internos
    virtual void close() = 0;

    // Retorna a lista de nomes das páginas ordenadas
    virtual const std::vector<std::string>& getPages() const = 0;

    // Retorna a quantidade total de páginas
    virtual size_t getPageCount() const = 0;

    // Extrai uma página específica para um arquivo no disco
    virtual bool extractPage(size_t index, const std::string& outPath) = 0;

    // Extrai a capa (primeira página de imagem) para o destino especificado
    virtual bool extractCover(const std::string& outPath) = 0;

    // Carrega a textura diretamente da página na memória (se suportado pelo formato)
    virtual vita2d_texture* loadPageTexture(size_t index) = 0;
};

#endif // PARSE_H
