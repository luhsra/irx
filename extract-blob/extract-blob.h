#pragma once
#include <stdint.h>
#include <string>
#include <list>

struct Symbol {
    std::string name;
    size_t addr;
};

struct Section {
    std::string name;
    size_t addr;
    size_t size;
    std::list<Symbol> symbols;
};


std::list<Section> map_parse(std::string map_filename);

struct Section* map_get_section_of_symbol(std::list<Section> &sections, std::string symbol, size_t *offset = nullptr);
