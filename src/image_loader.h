#pragma once
#include <vita2d.h>
#include <string>

namespace ImageLoader {

// Carrega imagem (JPG, PNG, WEBP, BMP, etc) a partir de um arquivo no disco
// utilizando a pipeline segura do MuPDF e criando a textura via vita2d
vita2d_texture* loadTextureFromFile(const std::string& filePath);

// Carrega imagem a partir de buffer em memória
vita2d_texture* loadTextureFromBuffer(const unsigned char* buffer, size_t size);

// Carrega miniatura a partir de um arquivo no disco com resolução limitada
vita2d_texture* loadThumbnailFromFile(const std::string& filePath, int maxW = 90, int maxH = 130);

// Carrega miniatura a partir de buffer em memória com resolução limitada
vita2d_texture* loadThumbnailFromBuffer(const unsigned char* buffer, size_t size, int maxW = 90, int maxH = 130);

// Obtém as dimensões originais da imagem para checar se é página dupla (width > height)
bool getImageDimensionsFromFile(const std::string& filePath, int& outW, int& outH);

// Redimensiona uma imagem do disco e salva diretamente como arquivo compacto (JPEG/PNG)
bool saveResizedThumbnail(const std::string& imagePath, const std::string& outThumbnailPath, int maxW = 120, int maxH = 170);

// Gera a miniatura de capa para um livro/mangá (CBZ, CBR, CBT, EPUB, PDF, etc.)
bool generateCoverThumbnail(const std::string& bookPath, const std::string& outThumbnailPath, int maxW = 120, int maxH = 170);

}
