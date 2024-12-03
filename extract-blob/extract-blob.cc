#include <llvm/Support/CommandLine.h>
#include <stdio.h>
#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>
#include <unistd.h>
#include <vector>
#include <memory>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <unordered_set>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Debug.h>

#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Host.h"

#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Linker/Linker.h>

#define DEBUG_TYPE "Blob-extractor"

#include "extract-blob.h"

using namespace std;
using namespace llvm;

Module* collectLinkerScriptSections(Module *M);

vector<string> external_symbols = {
  "__native_PRE_BOOT_1_tasks_start", "__native_PRE_BOOT_2_tasks_start", "__native_PRE_BOOT_3_tasks_start", "__native_FIRST_SLEEP_tasks_start", "__native_ON_EXIT_tasks_start", "__native_tasks_end", "__bss_start", "__bss_end", "__init_PRE_KERNEL_1_start", "__init_PRE_KERNEL_2_start", "__init_POST_KERNEL_start", "__init_APPLICATION_start", "__init_end",
  
  "__deferred_init_list_start",
  "__deferred_init_list_end",
  "__init_EARLY_start",
  "__static_thread_data_list_start", "__static_thread_data_list_end",
  "_device_list_start", "_device_list_end",
  "__device_init_status_start", "__device_start", "__device_end",

// "_k_heap_list_start", "_k_heap_list_end",
// "stderr", "stdout",
};

struct ElfExtractor {
  Elf *elf = NULL;
  int fd = 0;
};


struct ElfExtractor ee_open(const char *name) {
  struct ElfExtractor ee {};
  elf_version(EV_CURRENT);
  ee.fd = open(name, O_RDONLY, 0);
  ee.elf = elf_begin(ee.fd, ELF_C_READ, NULL);
  return ee;
}

void ee_close(struct ElfExtractor &ee) {
    elf_end(ee.elf);
    close(ee.fd);
}

Elf_Scn* ee_get_section(struct ElfExtractor &ee, GElf_Shdr *shdr, const char *section_name, Elf64_Word sh_type = 0, Elf64_Word addr = 0) {
    char *scn_name = NULL;
    Elf_Scn *scn = NULL;
    size_t shstrndx = 0;
    
    elf_getshdrstrndx(ee.elf, &shstrndx);

    while ((scn = elf_nextscn(ee.elf, scn)) != NULL) {
        gelf_getshdr(scn, shdr);
        
        scn_name = elf_strptr(ee.elf, shstrndx, shdr->sh_name);
        
        if (section_name != nullptr && strcmp(scn_name, section_name) == 0) {
          return scn;
        }
        
        if (shdr->sh_type == sh_type) {
          return scn;
        }
        
        if(addr != 0) {
          if(addr >= shdr->sh_addr && addr < shdr->sh_addr + shdr->sh_size) {
            LLVM_DEBUG(dbgs() << "FOUND " << scn_name << " "
              << addr << " " << shdr->sh_addr << " " << shdr->sh_size << '\n');
            return scn;
          }
        }
    }
    
    LLVM_DEBUG(dbgs() << "extract-blob: section not found: " << section_name << "\n");
    
    return nullptr;
}

vector<Constant*> section_to_llvm_data(struct ElfExtractor &ee, LLVMContext &Context, string section) {
  vector<Constant*> values;
  auto type = IntegerType::getInt8Ty(Context);
  GElf_Shdr shdr;
  Elf_Scn *scn = ee_get_section(ee, &shdr, section.c_str());
  Elf_Data *data = NULL;
  
  while ((data = elf_getdata(scn, data)) != NULL) {
    char *ptr_data = (char*) data->d_buf;
    if(ptr_data) {
      for(size_t i = 0; i < data->d_size; ++i) {
        values.push_back(ConstantInt::getIntegerValue(type, APInt(8, ptr_data[i])));
      }
    }
  }
  
  return values;
}

bool setDataLayout(Module *M, std::string arch) {
    Triple triple(sys::getDefaultTargetTriple());
    
    std::string Error;
    const Target *target = TargetRegistry::lookupTarget(arch, triple, Error);
    if (!target) {
        errs() << "Invalid machine: TargetRegistry::lookupTarget: " << Error << "\n";
        llvm::TargetRegistry::printRegisteredTargetsForVersion(outs());
        return false;
    }
        
    TargetOptions opt;
    auto TM = std::unique_ptr<TargetMachine>(
      target->createTargetMachine(triple.getTriple(), "generic", "", opt, None));
    
    M->setDataLayout(TM->createDataLayout());
    M->setTargetTriple(triple.getTriple());
    return true;
}

bool extract_blob(Module *Module, string InputFilename) {
    LLVMContext &Context = Module->getContext();
    string map_filename = InputFilename;
    std::size_t found = map_filename.rfind(".elf");
    if(found == string::npos) {
        errs() << "extract-blob: Input filename needs to end with the file extention '.elf'" << "\n";
        return false;
    }
    
    map_filename.replace(found, map_filename.length(), ".map");
    
    // parse the sections from the linker.map file
    list<Section> map_sections = map_parse(map_filename);
    
    
    struct ElfExtractor ee = ee_open(InputFilename.c_str());
    
    unordered_set<string> sections;
    for(auto sym_name : external_symbols) {
      LLVM_DEBUG(dbgs() << sym_name << " ... ");
      struct Section* scn = map_get_section_of_symbol(map_sections, sym_name.c_str());
      if(scn) {
        string section_name = scn->name;
        
        if(section_name.size()) {
          sections.insert(section_name);
          LLVM_DEBUG(dbgs() << "found!");
        }
      } else {
        LLVM_DEBUG(dbgs() << " section not found of symbol");
      }
      LLVM_DEBUG(dbgs() << '\n');
    }
    
    IRBuilder<> Builder(Context);

    auto type = IntegerType::getInt8Ty(Context);
    auto typePtr = PointerType::get(type, 0);
    
    // place the sections as binary blobs into the IR file
    for(auto section : sections) {
      vector<Constant*> values = section_to_llvm_data(ee, Context, section);
      auto *packedTy = ArrayType::get(type, values.size());
      auto *globalVariable = new GlobalVariable(*Module, packedTy, false, GlobalVariable::ExternalLinkage, nullptr, section);

      globalVariable->setDSOLocal(true);
      globalVariable->setConstant(true);
      globalVariable->setInitializer(ConstantArray::get(packedTy, values));
    }
    
    // set up the symbols using GEP to reference the memory from the sections binary blobs
    for(auto sym_name : external_symbols) {
      size_t offset[2] = {0, 0};
      struct Section* scn = map_get_section_of_symbol(map_sections, sym_name.c_str(), &offset[1]);
      if(!scn)
        continue;
      string section_name = scn->name;
      
      if(offset[1] > scn->size)
        continue;
      
      // special case: end of section. Semantically equal
      if(offset[1] == scn->size) {
        offset[0] = 1;
        offset[1] = 0;
      }
      
      auto globalVariable = Module->getNamedGlobal(section_name);
      if(!globalVariable)
        continue;
      
      auto *packedTy = ArrayType::get(type, scn->size);
      
      Module->getOrInsertGlobal(sym_name, typePtr);
      auto globalVariable_start = Module->getNamedGlobal(sym_name);
      
      auto *GEP = ConstantExpr::getInBoundsGetElementPtr(packedTy, globalVariable, ArrayRef<Value*> {
          ConstantInt::getIntegerValue(type, APInt(64, offset[0])),
          ConstantInt::getIntegerValue(type, APInt(64, offset[1])),
      });
      globalVariable_start->setInitializer(GEP);
      globalVariable_start->setDSOLocal(true);
    }
    
    ee_close(ee);
    return true;
}

unique_ptr<Module> collectSections(Module *Module, string InputFilename) {
    SMDiagnostic Err;
    LLVMContext &Context = Module->getContext();
    auto InputModule = llvm::parseIRFile(InputFilename, Err, Context);
    
    // Linker::linkModules(*InputModule, unique_ptr<llvm::Module>(MG), Linker::Flags::OverrideFromSrc);
    // Linker::linkModules(*Module, std::move(InputModule));
    
    Linker::linkModules(*Module, std::move(InputModule));
    llvm::Module *MG = collectLinkerScriptSections(Module);
    // Linker::linkModules(*Module, unique_ptr<llvm::Module>(MG), Linker::Flags::OverrideFromSrc);
    
    return unique_ptr<llvm::Module>(nullptr);
}

cl::opt<string> InputFilename(cl::Positional, cl::desc("<ELF input file>"), cl::value_desc("filename"), cl::Required);
cl::opt<string> OutputFilename("o", cl::desc("LLVM-IR output file"), cl::value_desc("filename"), cl::init("-"));
cl::opt<string> Arch("m", cl::desc("Target arch"), cl::value_desc("arch"), cl::init("x86"));

int main(int argc, char* argv[]) {
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();
    
    error_code EC;
    cl::ParseCommandLineOptions(argc, argv, "Linker emulator to  collect sections or extract blobs for elf files\n");
        
    raw_ostream *Output = &outs();
    
    if (OutputFilename != "-") {
      raw_fd_ostream *out = new raw_fd_ostream(OutputFilename.c_str(), EC);
      
      if (out->has_error()) {
        errs() << "extract-blob: Could not create output file" << "\n";
        out->close();
        delete out;
        return -1;
      }
      
      Output = out;
    }
    
    LLVMContext Context;
    Module *Module = new llvm::Module("extract-blob", Context);
    
    setDataLayout(Module, Arch);
    
    // extract_blob(Module, InputFilename);
    collectSections(Module, InputFilename);
    
    verifyModule(*Module, &errs());
    
    Module->print(*Output, nullptr);
    if(raw_fd_ostream *O = dynamic_cast<raw_fd_ostream*>(Output)) {
      O->close();
      delete O;
    }

    delete Module;
    return 0;
}
