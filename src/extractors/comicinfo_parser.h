#pragma once

#include "detail_metadata.h"
#include <string>

namespace Extractor {

class ComicInfoParser {
public:
    /**
     * Extrai metadados completos do ComicInfo.xml e subpastas de capítulos
     * para arquivos de mangá/HQ (CBZ, CBR, CBT, ZIP, RAR, TAR).
     * Não descompacta as páginas em disco.
     */
    static bool parse(const std::string& filePath, DetailMetadata& outDetail);

private:
    static bool parseZip(const std::string& filePath, DetailMetadata& outDetail);
    static bool parseRar(const std::string& filePath, DetailMetadata& outDetail);
    static bool parseTar(const std::string& filePath, DetailMetadata& outDetail);
    static bool parseXmlData(const uint8_t* xmlData, size_t xmlSize, DetailMetadata& outDetail);
    static void splitString(const std::string& str, char delimiter, std::vector<std::string>& outList);
    static std::string trim(const std::string& str);
};

} // namespace Extractor

