#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>

extern "C" {
#include <mupdf/fitz.h>
}

namespace Extractor {

class ArchiveHelper {
public:
    ArchiveHelper();
    ~ArchiveHelper();

    bool open(const std::string& archivePath);
    void close();

    bool hasEntry(const std::string& entryName) const;
    int countEntries() const;
    std::string getEntryName(int index) const;

    bool readEntry(const std::string& entryName, std::vector<uint8_t>& outData) const;
    bool readEntryString(const std::string& entryName, std::string& outText) const;

    fz_xml* parseXmlEntry(const std::string& entryName) const;
    fz_xml* parseXmlBuffer(const uint8_t* data, size_t size) const;

    fz_context* getCtx() const { return m_ctx; }
    fz_archive* getArchive() const { return m_archive; }

private:
    fz_context* m_ctx = nullptr;
    fz_archive* m_archive = nullptr;
};

} // namespace Extractor

