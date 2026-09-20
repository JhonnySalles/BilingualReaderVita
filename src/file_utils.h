#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>
#include <vector>

namespace FileUtils {

// Retorna extensão em minúsculo (ex: "cbz", "zip", "rar")
std::string getExtension(const std::string& path);

// Retorna apenas o nome do arquivo a partir de um path
std::string getFileName(const std::string& path);

// Verifica se a extensão é uma imagem suportada (png, jpg, jpeg, webp, bmp)
bool isImageFile(const std::string& filename);

// Ordenação alfanumérica natural (ex: 1.jpg, 2.jpg, 10.jpg)
bool naturalSortCompare(const std::string& a, const std::string& b);

// Cria diretório se não existir (suporta recursivo no Vita)
bool makeDir(const std::string& path);
bool makeDirRecursive(const std::string& path);

// Remove diretório e seu conteúdo recursivamente
bool removeDirRecursive(const std::string& path);

// Copia arquivo de src para dst
bool copyFile(const std::string& src, const std::string& dst);

// Gera uma string randômica alfanumérica (ex: para sessões de leitura no cache)
std::string generateRandomId(size_t length = 8);

// Lê os primeiros bytes de um arquivo para detecção de Magic Bytes
bool readMagicBytes(const std::string& path, unsigned char* buffer, size_t size);

} // namespace FileUtils

#endif // FILE_UTILS_H
