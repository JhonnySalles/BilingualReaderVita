#include "epub_packager.h"
#include "zip_writer.h"
#include <sstream>
#include <algorithm>

namespace Extractor {

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::string EpubPackager::getMimeType(const std::string& fileName) {
    size_t dot = fileName.find_last_of('.');
    if (dot == std::string::npos) return "application/octet-stream";
    std::string ext = toLower(fileName.substr(dot + 1));
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "png") return "image/png";
    if (ext == "gif") return "image/gif";
    if (ext == "webp") return "image/webp";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "css") return "text/css";
    if (ext == "xhtml" || ext == "html" || ext == "htm") return "application/xhtml+xml";
    if (ext == "ncx") return "application/x-dtbncx+xml";
    if (ext == "otf") return "font/otf";
    if (ext == "ttf") return "font/ttf";
    if (ext == "woff") return "font/woff";
    if (ext == "woff2") return "font/woff2";
    return "application/octet-stream";
}

bool EpubPackager::createEpub(
    const std::string& outputFilePath,
    const std::string& title,
    const std::string& author,
    const std::vector<uint8_t>& coverBytes,
    const std::string& coverFileName,
    const std::vector<GalleryItem>& galleryItems,
    const std::vector<ChapterItem>& chapters,
    const std::map<std::string, std::vector<uint8_t>>& resourceFiles,
    const std::string& customCss
) {
    ZipWriter zip;
    if (!zip.open(outputFilePath)) {
        return false;
    }

    // 1. mimetype (MUST be first, uncompressed STORE)
    const std::string mimetype = "application/epub+zip";
    zip.addEntry("mimetype", reinterpret_cast<const uint8_t*>(mimetype.data()), mimetype.size(), false);

    // 2. META-INF/container.xml
    const std::string containerXml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<container version=\"1.0\" xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\">\n"
        "  <rootfiles>\n"
        "    <rootfile full-path=\"OEBPS/content.opf\" media-type=\"application/oebps-package+xml\"/>\n"
        "  </rootfiles>\n"
        "</container>";
    zip.addEntry("META-INF/container.xml", containerXml, false);

    // 3. OEBPS/reader-base.css
    std::string css = customCss.empty() ?
        "body { margin: 0; padding: 0; }\n"
        "p { margin: 1em 0; line-height: 1.5; }\n"
        "h1, h2, h3 { text-align: center; margin: 1.2em 0; }\n"
        "img { max-width: 100%; height: auto; }\n"
        ".br-full-page { text-align: center; margin: 0; padding: 0; page-break-inside: avoid; }\n"
        ".br-full-page img { max-width: 100%; max-height: 100%; width: auto; height: auto; display: block; margin: 0 auto; }\n"
        ".br-inline { display: inline; vertical-align: middle; }\n"
        ".br-inline-wrapper { text-align: center; margin: 0.8em 0; }\n"
        ".br-gap { margin: 1.5em 0; }\n"
        : customCss;

    zip.addEntry("OEBPS/reader-base.css", css, false);
    zip.addEntry("OEBPS/style.css", css, false);

    std::ostringstream manifest;
    std::ostringstream spine;
    std::ostringstream navMap;
    int playOrder = 1;

    manifest << "    <item id=\"reader-base-css\" href=\"reader-base.css\" media-type=\"text/css\" />\n";
    manifest << "    <item id=\"style\" href=\"style.css\" media-type=\"text/css\" />\n";

    // 4. Cover
    bool hasCover = !coverBytes.empty();
    if (hasCover) {
        std::string coverMime = getMimeType(coverFileName);
        zip.addEntry("OEBPS/" + coverFileName, coverBytes, false);
        manifest << "    <item id=\"cover-image\" href=\"" << coverFileName << "\" media-type=\"" << coverMime << "\" />\n";

        std::string coverXhtml =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
            "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n"
            "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
            "<head>\n"
            "  <title>Cover</title>\n"
            "  <link rel=\"stylesheet\" href=\"reader-base.css\" type=\"text/css\" />\n"
            "  <style type=\"text/css\">\n"
            "    body { margin: 0; padding: 0; text-align: center; }\n"
            "    img { max-width: 100%; max-height: 100%; width: auto; height: auto; display: block; margin: 0 auto; }\n"
            "  </style>\n"
            "</head>\n"
            "<body>\n"
            "  <div class=\"br-full-page\">\n"
            "    <img src=\"" + coverFileName + "\" alt=\"Cover\" />\n"
            "  </div>\n"
            "</body>\n"
            "</html>";

        zip.addEntry("OEBPS/cover.xhtml", coverXhtml, false);
        manifest << "    <item id=\"cover-page\" href=\"cover.xhtml\" media-type=\"application/xhtml+xml\" />\n";
        spine << "    <itemref idref=\"cover-page\" />\n";

        navMap << "    <navPoint id=\"navPoint-" << playOrder << "\" playOrder=\"" << playOrder << "\">\n";
        navMap << "      <navLabel><text>Cover</text></navLabel>\n";
        navMap << "      <content src=\"cover.xhtml\" />\n";
        navMap << "    </navPoint>\n";
        playOrder++;
    }

    // 5. Gallery
    for (size_t i = 0; i < galleryItems.size(); ++i) {
        std::string galFileName = "gallery_" + std::to_string(i) + ".xhtml";
        std::string galId = "gallery-" + std::to_string(i);

        std::string galXhtml =
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
            "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n"
            "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
            "<head>\n"
            "  <title>" + SpineSegmenter::escapeXml(galleryItems[i].title) + "</title>\n"
            "  <link rel=\"stylesheet\" href=\"reader-base.css\" type=\"text/css\" />\n"
            "</head>\n"
            "<body>\n"
            "  <div class=\"br-full-page\">\n"
            "    <img src=\"" + SpineSegmenter::escapeXml(galleryItems[i].imageFileName) + "\" alt=\"Illustration\" />\n"
            "  </div>\n"
            "</body>\n"
            "</html>";

        zip.addEntry("OEBPS/" + galFileName, galXhtml, false);
        manifest << "    <item id=\"" << galId << "\" href=\"" << galFileName << "\" media-type=\"application/xhtml+xml\" />\n";
        spine << "    <itemref idref=\"" << galId << "\" />\n";

        navMap << "    <navPoint id=\"navPoint-" << playOrder << "\" playOrder=\"" << playOrder << "\">\n";
        navMap << "      <navLabel><text>" << SpineSegmenter::escapeXml(galleryItems[i].title) << "</text></navLabel>\n";
        navMap << "      <content src=\"" << galFileName << "\" />\n";
        navMap << "    </navPoint>\n";
        playOrder++;
    }

    // 6. Chapters
    for (size_t i = 0; i < chapters.size(); ++i) {
        const auto& chap = chapters[i];
        std::string chapId = "chap-" + std::to_string(i);
        std::string chapFileName = chap.fileName.empty() ? ("chap_" + std::to_string(i) + ".xhtml") : chap.fileName;

        std::ostringstream chapXhtml;
        chapXhtml << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
        chapXhtml << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n";
        chapXhtml << "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n";
        chapXhtml << "<head>\n";
        chapXhtml << "  <title>" << SpineSegmenter::escapeXml(chap.title) << "</title>\n";
        chapXhtml << "  <link rel=\"stylesheet\" href=\"reader-base.css\" type=\"text/css\" />\n";
        for (const auto& cssLink : chap.cssLinks) {
            chapXhtml << "  <link rel=\"stylesheet\" href=\"" << SpineSegmenter::escapeXml(cssLink) << "\" type=\"text/css\" />\n";
        }
        chapXhtml << "</head>\n";
        chapXhtml << "<body>\n";
        if (!chap.anchorId.empty()) {
            chapXhtml << "<a id=\"" << SpineSegmenter::escapeXml(chap.anchorId) << "\"></a>\n";
        }
        chapXhtml << chap.contentHtml << "\n";
        chapXhtml << "</body>\n";
        chapXhtml << "</html>";

        zip.addEntry("OEBPS/" + chapFileName, chapXhtml.str(), false);
        manifest << "    <item id=\"" << chapId << "\" href=\"" << chapFileName << "\" media-type=\"application/xhtml+xml\" />\n";
        spine << "    <itemref idref=\"" << chapId << "\" />\n";

        if (!chap.title.empty()) {
            navMap << "    <navPoint id=\"navPoint-" << playOrder << "\" playOrder=\"" << playOrder << "\">\n";
            navMap << "      <navLabel><text>" << SpineSegmenter::escapeXml(chap.title) << "</text></navLabel>\n";
            navMap << "      <content src=\"" << chapFileName;
            if (!chap.anchorId.empty()) {
                navMap << "#" << SpineSegmenter::escapeXml(chap.anchorId);
            }
            navMap << "\" />\n";
            navMap << "    </navPoint>\n";
            playOrder++;
        }
    }

    // 7. Resource files (Images, CSS, Fonts)
    int resIndex = 0;
    for (const auto& pair : resourceFiles) {
        std::string resPath = pair.first;
        while (!resPath.empty() && (resPath.front() == '/' || resPath.front() == '\\')) {
            resPath.erase(0, 1);
        }
        std::string resId = "res-" + std::to_string(resIndex++);
        std::string resMime = getMimeType(resPath);

        zip.addEntry("OEBPS/" + resPath, pair.second, false);
        manifest << "    <item id=\"" << resId << "\" href=\"" << SpineSegmenter::escapeXml(resPath)
                 << "\" media-type=\"" << resMime << "\" />\n";
    }

    // NCX item in manifest
    manifest << "    <item id=\"ncx\" href=\"toc.ncx\" media-type=\"application/x-dtbncx+xml\" />\n";

    // 8. OEBPS/content.opf
    std::string safeTitle = title.empty() ? "Untitled Book" : title;
    std::string safeAuthor = author.empty() ? "Unknown Author" : author;

    std::ostringstream opf;
    opf << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    opf << "<package xmlns=\"http://www.idpf.org/2007/opf\" version=\"2.0\" unique-identifier=\"BookId\">\n";
    opf << "  <metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:opf=\"http://www.idpf.org/2007/opf\">\n";
    opf << "    <dc:title>" << SpineSegmenter::escapeXml(safeTitle) << "</dc:title>\n";
    opf << "    <dc:creator>" << SpineSegmenter::escapeXml(safeAuthor) << "</dc:creator>\n";
    opf << "    <dc:language>en</dc:language>\n";
    opf << "    <dc:identifier id=\"BookId\">urn:uuid:bilingual-vita-book</dc:identifier>\n";
    if (hasCover) {
        opf << "    <meta name=\"cover\" content=\"cover-image\" />\n";
    }
    opf << "  </metadata>\n";
    opf << "  <manifest>\n";
    opf << manifest.str();
    opf << "  </manifest>\n";
    opf << "  <spine toc=\"ncx\">\n";
    opf << spine.str();
    opf << "  </spine>\n";
    if (hasCover) {
        opf << "  <guide>\n";
        opf << "    <reference type=\"cover\" title=\"Cover\" href=\"cover.xhtml\" />\n";
        opf << "  </guide>\n";
    }
    opf << "</package>";

    zip.addEntry("OEBPS/content.opf", opf.str(), false);

    // 9. OEBPS/toc.ncx
    std::ostringstream ncx;
    ncx << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
    ncx << "<!DOCTYPE ncx PUBLIC \"-//NISO//DTD ncx 2005-1//EN\" \"http://www.daisy.org/z3986/2005/ncx-2005-1.dtd\">\n";
    ncx << "<ncx xmlns=\"http://www.daisy.org/z3986/2005/ncx/\" version=\"2005-1\">\n";
    ncx << "  <head>\n";
    ncx << "    <meta name=\"dtb:uid\" content=\"urn:uuid:bilingual-vita-book\" />\n";
    ncx << "    <meta name=\"dtb:depth\" content=\"1\" />\n";
    ncx << "    <meta name=\"dtb:totalPageCount\" content=\"0\" />\n";
    ncx << "    <meta name=\"dtb:maxPageNumber\" content=\"0\" />\n";
    ncx << "  </head>\n";
    ncx << "  <docTitle><text>" << SpineSegmenter::escapeXml(safeTitle) << "</text></docTitle>\n";
    ncx << "  <navMap>\n";
    ncx << navMap.str();
    ncx << "  </navMap>\n";
    ncx << "</ncx>";

    zip.addEntry("OEBPS/toc.ncx", ncx.str(), false);

    return zip.close();
}

} // namespace Extractor

