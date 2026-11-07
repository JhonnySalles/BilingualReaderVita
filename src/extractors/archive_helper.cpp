#include "archive_helper.h"
#include <cstring>

namespace Extractor {

ArchiveHelper::ArchiveHelper() {
    m_ctx = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
    if (m_ctx) {
        fz_register_document_handlers(m_ctx);
    }
}

ArchiveHelper::~ArchiveHelper() {
    close();
    if (m_ctx) {
        fz_drop_context(m_ctx);
        m_ctx = nullptr;
    }
}

bool ArchiveHelper::open(const std::string& archivePath) {
    close();
    if (!m_ctx) return false;

    fz_try(m_ctx) {
        m_archive = fz_open_archive(m_ctx, archivePath.c_str());
    } fz_catch(m_ctx) {
        m_archive = nullptr;
    }

    return m_archive != nullptr;
}

void ArchiveHelper::close() {
    if (m_archive && m_ctx) {
        fz_drop_archive(m_ctx, m_archive);
        m_archive = nullptr;
    }
}

bool ArchiveHelper::hasEntry(const std::string& entryName) const {
    if (!m_archive || !m_ctx) return false;
    return fz_has_archive_entry(m_ctx, m_archive, entryName.c_str()) != 0;
}

int ArchiveHelper::countEntries() const {
    if (!m_archive || !m_ctx) return 0;
    return fz_count_archive_entries(m_ctx, m_archive);
}

std::string ArchiveHelper::getEntryName(int index) const {
    if (!m_archive || !m_ctx) return "";
    const char* name = fz_list_archive_entry(m_ctx, m_archive, index);
    return name ? std::string(name) : "";
}

bool ArchiveHelper::readEntry(const std::string& entryName, std::vector<uint8_t>& outData) const {
    if (!m_archive || !m_ctx) return false;
    fz_buffer* buf = nullptr;

    fz_try(m_ctx) {
        buf = fz_read_archive_entry(m_ctx, m_archive, entryName.c_str());
    } fz_catch(m_ctx) {
        buf = nullptr;
    }

    if (!buf) return false;

    unsigned char* data = nullptr;
    size_t len = fz_buffer_storage(m_ctx, buf, &data);
    outData.assign(data, data + len);
    fz_drop_buffer(m_ctx, buf);
    return true;
}

bool ArchiveHelper::readEntryString(const std::string& entryName, std::string& outText) const {
    std::vector<uint8_t> data;
    if (!readEntry(entryName, data)) return false;
    outText.assign(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

fz_xml* ArchiveHelper::parseXmlEntry(const std::string& entryName) const {
    if (!m_archive || !m_ctx) return nullptr;
    fz_xml* xml = nullptr;

    fz_try(m_ctx) {
        xml = fz_parse_xml_archive_entry(m_ctx, m_archive, entryName.c_str(), 0);
    } fz_catch(m_ctx) {
        xml = nullptr;
    }

    return xml;
}

fz_xml* ArchiveHelper::parseXmlBuffer(const uint8_t* data, size_t size) const {
    if (!m_ctx || !data || size == 0) return nullptr;
    fz_xml* xml = nullptr;
    fz_buffer* buf = nullptr;

    fz_try(m_ctx) {
        buf = fz_new_buffer_from_copied_data(m_ctx, data, size);
        xml = fz_parse_xml(m_ctx, buf, 0);
    } fz_always(m_ctx) {
        fz_drop_buffer(m_ctx, buf);
    } fz_catch(m_ctx) {
        xml = nullptr;
    }

    return xml;
}

} // namespace Extractor

