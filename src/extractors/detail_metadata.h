#pragma once

#include <string>
#include <vector>

struct ChapterItem {
    std::string title;
    int pageIndex = 0;       // Índice baseado em 0 no leitor
    bool isBookmark = false; // Marcador especial do ComicInfo.xml (<Page Bookmark="...">)
};

struct DetailMetadata {
    bool isLoaded = false;
    bool isManga = false;

    // Identificação Geral
    std::string title;
    std::string series;
    std::string number;
    int volume = 0;
    int count = 0;
    std::string publisher;
    std::string imprint;
    std::string releaseDate;
    std::string genre;
    std::string language;
    std::string isbn;
    std::string ageRating;
    std::string mangaDirection;
    std::string storyArc;
    std::string seriesGroup;
    std::string web;

    // Equipe Criativa e Autoria
    std::string authors;
    std::string writer;
    std::string penciller;
    std::string inker;
    std::string colorist;
    std::string letterer;
    std::string coverArtist;
    std::string editor;
    std::string translator;

    // Sinopse e Notas (texto limpo de tags HTML)
    std::string summary;
    std::string notes;

    // Coleções de Tags / Chips
    std::vector<std::string> genres;
    std::vector<std::string> tags;
    std::vector<std::string> characters;
    std::vector<std::string> teams;
    std::vector<std::string> locations;

    // Capítulos e Marcadores de Página
    std::vector<ChapterItem> chapters;

    // Informações de Vínculo Bilíngue (PagesLink)
    bool hasLinkedFile = false;
    std::string linkedFileName;
    std::string linkedFilePath;
    int linkedPageCount = 0;
};

