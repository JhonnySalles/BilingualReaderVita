#include "parse_factory.h"
#include "parse_mupdf.h"
#include "parse_rar.h"
#include "parse_tar.h"
#include "../file_utils.h"
#include <algorithm>

Parse* ParseFactory::create(const std::string& path) {
    if (path.empty()) return nullptr;

    std::string ext = FileUtils::getExtension(path);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);

    Parse* parse = nullptr;

    if (ext == "CBR" || ext == "RAR") {
        parse = new ParseRar();
    } else if (ext == "CBT" || ext == "TAR") {
        parse = new ParseTar();
    } else {
        // CBZ, ZIP, PDF, EPUB, etc.
        parse = new ParseMuPDF();
    }

    if (parse) {
        if (!parse->open(path)) {
            delete parse;
            parse = nullptr;
        }
    }

    return parse;
}

void ParseFactory::destroy(Parse*& parse) {
    if (parse) {
        parse->close();
        delete parse;
        parse = nullptr;
    }
}
