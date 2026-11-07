#include "zip_writer.h"
#include <zlib.h>
#include <cstring>

namespace Extractor {

ZipWriter::ZipWriter() = default;

ZipWriter::~ZipWriter() {
    close();
}

bool ZipWriter::open(const std::string& outputPath) {
    close();
    m_file = fopen(outputPath.c_str(), "wb");
    if (!m_file) return false;
    m_entries.clear();
    m_currentOffset = 0;
    return true;
}

static void writeU16(FILE* f, uint16_t v) {
    uint8_t b[2] = { static_cast<uint8_t>(v & 0xFF), static_cast<uint8_t>((v >> 8) & 0xFF) };
    fwrite(b, 1, 2, f);
}

static void writeU32(FILE* f, uint32_t v) {
    uint8_t b[4] = {
        static_cast<uint8_t>(v & 0xFF),
        static_cast<uint8_t>((v >> 8) & 0xFF),
        static_cast<uint8_t>((v >> 16) & 0xFF),
        static_cast<uint8_t>((v >> 24) & 0xFF)
    };
    fwrite(b, 1, 4, f);
}

bool ZipWriter::addEntry(const std::string& entryName, const uint8_t* data, size_t size, bool compress) {
    if (!m_file) return false;

    EntryInfo info;
    info.name = entryName;
    info.uncompressedSize = static_cast<uint32_t>(size);
    info.localHeaderOffset = m_currentOffset;
    info.crc = static_cast<uint32_t>(crc32(0L, data ? data : reinterpret_cast<const uint8_t*>(""), static_cast<uInt>(size)));

    std::vector<uint8_t> compressedData;
    if (compress && size > 32) {
        uLongf destLen = compressBound(static_cast<uLong>(size));
        compressedData.resize(destLen);
        z_stream strm{};
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        // Raw deflate without zlib wrapper (-15)
        if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) == Z_OK) {
            strm.next_in = const_cast<Bytef*>(data);
            strm.avail_in = static_cast<uInt>(size);
            strm.next_out = compressedData.data();
            strm.avail_out = static_cast<uInt>(destLen);
            deflate(&strm, Z_FINISH);
            destLen = strm.total_out;
            deflateEnd(&strm);

            if (destLen < size) {
                compressedData.resize(destLen);
                info.compressionMethod = 8;
                info.compressedSize = static_cast<uint32_t>(destLen);
            } else {
                compressedData.clear();
                info.compressionMethod = 0;
                info.compressedSize = info.uncompressedSize;
            }
        } else {
            info.compressionMethod = 0;
            info.compressedSize = info.uncompressedSize;
        }
    } else {
        info.compressionMethod = 0;
        info.compressedSize = info.uncompressedSize;
    }

    // Write Local File Header (30 bytes + name)
    writeU32(m_file, 0x04034b50); // Signature
    writeU16(m_file, 20);         // Version needed (2.0)
    writeU16(m_file, 0);          // General purpose bit flag
    writeU16(m_file, info.compressionMethod);
    writeU16(m_file, 0);          // Last mod time
    writeU16(m_file, 0);          // Last mod date
    writeU32(m_file, info.crc);
    writeU32(m_file, info.compressedSize);
    writeU32(m_file, info.uncompressedSize);
    writeU16(m_file, static_cast<uint16_t>(entryName.size()));
    writeU16(m_file, 0);          // Extra field length
    fwrite(entryName.data(), 1, entryName.size(), m_file);

    m_currentOffset += 30 + static_cast<uint32_t>(entryName.size());

    // Write Payload
    const uint8_t* payload = info.compressionMethod == 8 ? compressedData.data() : data;
    if (info.compressedSize > 0 && payload) {
        fwrite(payload, 1, info.compressedSize, m_file);
        m_currentOffset += info.compressedSize;
    }

    m_entries.push_back(info);
    return true;
}

bool ZipWriter::addEntry(const std::string& entryName, const std::string& text, bool compress) {
    return addEntry(entryName, reinterpret_cast<const uint8_t*>(text.data()), text.size(), compress);
}

bool ZipWriter::addEntry(const std::string& entryName, const std::vector<uint8_t>& data, bool compress) {
    return addEntry(entryName, data.data(), data.size(), compress);
}

bool ZipWriter::close() {
    if (!m_file) return true;

    uint32_t centralDirStart = m_currentOffset;
    uint32_t centralDirSize = 0;

    // Write Central Directory Headers (46 bytes + name)
    for (const auto& entry : m_entries) {
        writeU32(m_file, 0x02014b50); // Central file header signature
        writeU16(m_file, 20);         // Version made by
        writeU16(m_file, 20);         // Version needed
        writeU16(m_file, 0);          // General purpose bit flag
        writeU16(m_file, entry.compressionMethod);
        writeU16(m_file, 0);          // Last mod time
        writeU16(m_file, 0);          // Last mod date
        writeU32(m_file, entry.crc);
        writeU32(m_file, entry.compressedSize);
        writeU32(m_file, entry.uncompressedSize);
        writeU16(m_file, static_cast<uint16_t>(entry.name.size()));
        writeU16(m_file, 0);          // Extra field length
        writeU16(m_file, 0);          // Comment length
        writeU16(m_file, 0);          // Disk number start
        writeU16(m_file, 0);          // Internal file attributes
        writeU32(m_file, 0);          // External file attributes
        writeU32(m_file, entry.localHeaderOffset);
        fwrite(entry.name.data(), 1, entry.name.size(), m_file);

        uint32_t headerLen = 46 + static_cast<uint32_t>(entry.name.size());
        m_currentOffset += headerLen;
        centralDirSize += headerLen;
    }

    // Write End of Central Directory Record (22 bytes)
    writeU32(m_file, 0x06054b50); // EOCD signature
    writeU16(m_file, 0);          // Number of this disk
    writeU16(m_file, 0);          // Disk where central directory starts
    writeU16(m_file, static_cast<uint16_t>(m_entries.size())); // Total records on this disk
    writeU16(m_file, static_cast<uint16_t>(m_entries.size())); // Total records
    writeU32(m_file, centralDirSize);
    writeU32(m_file, centralDirStart);
    writeU16(m_file, 0);          // Comment length

    fclose(m_file);
    m_file = nullptr;
    return true;
}

} // namespace Extractor

