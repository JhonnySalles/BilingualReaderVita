#pragma once
#include <vita2d.h>
#include <string>

namespace ImageLoader {

// Carrega imagem (JPG, PNG, WEBP, BMP, etc) a partir de um arquivo no disco
// utilizando a pipeline segura do MuPDF e criando a textura via vita2d
vita2d_texture* loadTextureFromFile(const std::string& filePath);

// Carrega imagem a partir de buffer em memória
vita2d_texture* loadTextureFromBuffer(const unsigned char* buffer, size_t size);

}
