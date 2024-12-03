#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <list>
#include <string>
#include <regex>
#include <fstream>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Debug.h>
#define DEBUG_TYPE "Blob-extractor-linker-map"

#include "extract-blob.h"

using namespace std;
using llvm::dbgs;

std::string getFileContent(const std::string& path) {
  std::ifstream file(path);
  return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

// reads a linker.map file and parses the section and its symbols
list<Section> map_parse(string map_filename) {
    list<Section> sections;
    string s = getFileContent(map_filename);
  
    std::regex section("^(\\w+)\\W+(0x[0-9a-f]+)\\W+(0x[0-9a-f]+)", std::regex_constants::optimize);
    std::regex symbol("^\\W*(0x[0-9a-f]+)\\W+(\\w+) = \\.", std::regex_constants::optimize);
    
    std::smatch match;
    size_t pos = 0;
    
    do {
      string str;
      size_t end = s.find('\n');
      if(end != std::string::npos) {
        end = s.find('\n', end + 1);
        if(end != std::string::npos) {
          str = s.substr(0, end + 1);
        }
      }
      
      if(str.length() == 0) {
        str = s;
      }
      
      if(std::regex_search(str, match, section)) {
        if(match.size() == 4) {
          size_t addr = std::stoul(match[2], nullptr, 16);
          size_t size = std::stoul(match[3], nullptr, 16);
          LLVM_DEBUG(dbgs() << "Section " << match[1].str() << " " << addr << " " << size << '\n');
          sections.push_back({match[1], addr, size, {}});
        }
        
        s = s.substr(match[0].length());
        
        continue;
      }
      
      if(std::regex_search(str, match, symbol)) {
        if(match.size() == 3) {
          size_t addr = std::stoul(match[1], nullptr, 16);
          LLVM_DEBUG(dbgs() << "\tsymbol " << match[2] << " " << addr << '\n');
          if(!sections.empty()) {
            sections.back().symbols.push_back({match[2], addr});
          }
        }
        
        s = s.substr(match[0].length());
        
        continue;
      }
      
      // consume garbage until new line
      pos = s.find('\n');
      if(pos != std::string::npos) {
        s = s.substr(pos+1);
      }
      
    } while(pos != std::string::npos);
    
    return sections;
}

struct Section* map_get_section_of_symbol(std::list<Section> &sections, string symbol, size_t *offset) {
    for(struct Section &scn : sections) {
        for(auto &sym : scn.symbols) {
            if(symbol.compare(sym.name) == 0) {
                if(offset) {
                  *offset = sym.addr - scn.addr;
                }
                return &scn;
            }
        }
    }
    return nullptr;
}
