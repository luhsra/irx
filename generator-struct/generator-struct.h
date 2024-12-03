#pragma once
#include <list>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/Support/raw_ostream.h>

llvm::DICompositeType* findStructDebugInfo(llvm::DebugInfoFinder &DIF, const char *name);

typedef std::list<struct Block*> Dependancies;

struct Block {
  Dependancies deps;
  bool printed = false;
};

struct Typedef: Block {
  // struct Block;
  llvm::DIDerivedType* typedefType;
  
  Typedef(llvm::DIDerivedType* typedefType): typedefType(typedefType) {}
};

struct Dataclass: Block {
    llvm::DICompositeType *name;
    llvm::DIDerivedType *DIDTy;
    
    Dataclass(llvm::DICompositeType *name): name(name) {}
};

struct GeneratorStruct {
  // list<DIDerivedType*> typedefs;
  // std::list<struct Block*> blocks;
  llvm::Module *M;
  
  llvm::raw_ostream *Output;
  
  // std::list<llvm::DIType*> stack;
  
  // std::list<Block*> blocks;
  
};

// algo:
// print everything with 0 dependacies
// remove deps pointers
// repeat
