#ifndef PARSE_FACTORY_H
#define PARSE_FACTORY_H

#include "parse.h"
#include <string>

class ParseFactory {
public:
    // Cria a instância de Parse adequada com base na extensão do arquivo e tenta abri-lo
    static Parse* create(const std::string& path);

    // Libera a instância de Parse e define o ponteiro como nullptr
    static void destroy(Parse*& parse);
};

#endif // PARSE_FACTORY_H
