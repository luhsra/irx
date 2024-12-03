#include <iostream>
#include <memory>
#include <numeric>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DebugInfo.h>

#include "irinterpreter2-struct.h"
#include "irinterpreter2-mmap.h"

// IRX Struct for Allocation Information and Setting Values in a struct

using namespace llvm;

#define PTR_ADD(PTR, OFFSET) ((void*)((size_t)PTR + OFFSET))

void print_bytes(char *ptr, size_t n);

std::list<struct TypedIndex> IRModuleInterpreter::GEPIndices(std::string name, std::list<std::string> path) {
    // DICompositeType* CTy = findStructDebugInfo(*M, name.c_str());
    // StructType *Ty = getTypeByName(name);
    // DICompositeType* CTy = findStructDebugInfo(*M, Ty->getName());


    // NOTE: just extend getStructFieldInfo to return the GEP indices
    // irx_print_struct already uses creates and uses GEP, but without debug info (no debug info is attached to it) => maybe it is possible to retrieve the debug info from the GEP indices

    std::vector<std::string> v{ std::begin(path), std::end(path) };

    // struct BasicTypeInfo info = getStructFieldInfo(name, v);
    struct BasicTypeInfo info = getStructFieldInfo2(name, v);

    // try to get all SType in info>path from the names .struct_types // this should be equal to `path`

    std::list<struct TypedIndex> typedIndices;

    // assert(info.struct_types.size() == info.gep.size());

    for(size_t i = 0; i < info.gep.size(); ++i) {
        // DBGS() << "GEPIndices type_name: " << info.struct_types.at(i) << "\n";
        // StructType *STy = getStructTypeByName();
        Type *Ty = nullptr;
        if(info.STys.size() > i) {
            Ty = info.STys.at(i);
            // if(STy == nullptr) {
            //     DBGS() << "ERROR: nullptr in STys at index" << i << "\n";
            // }
            // DBGS() << i << "\n";
        }
        
        typedIndices.push_back(TypedIndex{Ty, info.gep.at(i)});
    }
    
    assert(typedIndices.size() > 0);

    std::list<size_t> gep(info.gep.begin(), info.gep.end());

    return typedIndices;
}

// short method description: path => GEPIndices(path) => GEP2Offset(indices) => Offset
size_t IRModuleInterpreter::offsetInBitsFromPath(const char *name, std::vector<std::string> path) {
    StructType *Ty = getStructTypeByName(name);
    std::list<struct TypedIndex> indices = GEPIndices(name, std::list<std::string>{path.begin(), path.end()});
    size_t offset_bit = GEP2Offset(*M, Ty, indices);
    assert((offset_bit & 0x07) == 0 && "No support for bitfields");
    
    // assert(sizeof(unsigned long) * 8 == DITy->getSizeInBits() && "Size mismatch");
    
    std::string pathstr = "";
    for(size_t i = 0; i != path.size(); ++i) {
        pathstr = pathstr + path[i] + ".";
    }
    DBGS() << "offsetInBitsFromPath: " << pathstr << " offset_bit: " << offset_bit << " " << (offset_bit / 8) << "\n";
    
    return offset_bit;
}

// how to handle arrays? (all ellements at once)
// How to create a setter definition from a configuration string for dynamic nested structs => parseConstantString
// => Current solution it to make a call from Cython for every field, so this is unused
void IRModuleInterpreter::irx_struct_init(const char *name, void *m, struct FieldValue fields[], size_t numFields) {
    for(size_t i = 0; i < numFields; ++i) {
        FieldValue *f = &fields[i];
        DBGS() << "irx_struct_init irx_struct_set " << f->name << " size:" << f->size << "\n";

        irx_struct_set(name, m , {f->name}, f->value, f->size);
    }
}

std::vector<DIType*> IRModuleInterpreter::structInfo(std::string name) {
    return getStructDebugInfoFields(DIF, name.c_str());
}

size_t IRModuleInterpreter::irx_struct_get_size(const char* name) {
    StructType *Ty = getTypeByName(name);
    
    if(Ty == nullptr) {
        DBGS() << "struct name " << name << " not found in context" << "\n";
        return 0;
    }
    
    // return M->getDataLayout().getTypeAllocSize(Ty); // that would be usefull for arrays
    return M->getDataLayout().getTypeStoreSize(Ty);
}

// maybe: could also retrieve debug info for field names and return them
struct irx_alloc IRModuleInterpreter::structAlloc(std::string name) {
    auto alloc = I->malloc(this, name);

    DBGS() << "Allocate size: " << alloc.size << " byte for " << name << " v@" << alloc.vptr << "/vv@" << (void*) alloc.vvptr << "\n";
    
    return alloc;
}

// Funcionaly similar to StructType::getTypeByName. LLVM might add a postfix number to the sturct name.
// This function will search for a matching name and check for ambiguity.
StructType* getTypeByName(Module *M, std::string name) {
    StructType *Ty = nullptr;
    std::list<StructType*> matches;
    
    for(auto *S : M->getIdentifiedStructTypes()){
        std::string sname = S->getName().str();
        std::string striped_name = sname;
        size_t point = sname.rfind(".");
        if(point != std::string::npos) {
            striped_name = striped_name.substr(0, point);
        }
        
        if(sname.compare(name) == 0 || striped_name.compare(name) == 0) {
            matches.push_back(S);
        }
    }
    
    if(!matches.empty()) {
        Ty = matches.front();
        matches.pop_front();
        
        for(auto *S : matches) {
            if(!S->isLayoutIdentical(S)) {
                errs() << "ambiguous struct name " << name  << "\n";
                return nullptr;
            }
        }
    }
    return Ty;
}

StructType* IRModuleInterpreter::getTypeByName(std::string name) {
    auto it = STCache.find(name);
    if (it != STCache.end()) {
        return it->second;
    }
    
    StructType* STy = ::getTypeByName(M.get(), name);
    STCache[name] = STy;
    return STy;
}

StructType* IRModuleInterpreter::getStructTypeByName(std::string name) {
    std::string sname = "struct.";
    sname += name;
    StructType *Ty = getTypeByName(sname);
    if(!Ty) {
        errs() << "could not find struct: " << sname << "\n";
    }
    assert(Ty != nullptr && "Struct not found");
    return Ty;
}

void IRModuleInterpreter::irx_struct_set(const char *name, void *m,
                std::vector<std::string> path,
                void *value,
                size_t size) {
    m = vvptr2vptr((uintptr_t)m);
    size_t offset = offsetInBitsFromPath(name, path) / 8;
    
    // print_bytes((char*)PTR_ADD(m, offset), size);
    
    memcpy(PTR_ADD(m, offset), (char*) value, size);
    
    // print_bytes((char*)PTR_ADD(m, offset), size);
    
    // FIXME ENDIAN, need to swap bytes if needed, or use llvm function to write memory
    // StoreValueToMemory(Val, (GenericValue *)GVTOP(SRC),
                     // I.getOperand(0)->getType());
}

void IRModuleInterpreter::irx_struct_get(std::string name, void *m, std::vector<std::string> path, void *dest, size_t len) {
    m = vvptr2vptr((uintptr_t)m);
    size_t offset = offsetInBitsFromPath(name.c_str(), path) / 8;
    
    memcpy((char*) dest, PTR_ADD(m, offset), len);
}


size_t IRModuleInterpreter::GEP2Offset(Module &M, Type *Ty, std::list<TypedIndex> indices) {
    // This could be problematic if unions will get reordered
    // see: https://blog.yossarian.net/2020/09/19/LLVMs-getelementptr-by-example
    // instead of rely on the GEP index to retrieve the Type we should get it from the TypedIndex
    uint64_t Total = 0;
    Ty = ArrayType::get(Ty, indices.front().index+1);
    // auto container = indices.front();

    for (auto idx : indices) {
        
        if(idx.STy) {
            if(Ty != idx.STy) {
                // TODO only allow this for unions
                outs() << red << "GEP2Offset: Changing Type: ";
                // Ty->dump();
                outs() << " to ";
                // idx.STy->dump();
                outs() << reset << "\n";
                
                Ty = idx.STy;
            }
        }
        
        assert(Ty != nullptr && "Unknown struct type found while processing");
        // DBGS() << "GEP2Offset: idx: " << idx.index << " " << Ty->getTypeID() << "\n";
        // Ty->dump();
        
        if(Ty->getTypeID() == Type::StructTyID) {
            if(StructType *STy = dyn_cast<StructType>(Ty)) {
                // should probably use container FIXME
                assert(STy != nullptr && "Unknown struct type found while processing");
                // container = idx;
                
                const StructLayout *SLO = M.getDataLayout().getStructLayout(STy);
                Total += SLO->getElementOffset(idx.index);
                Ty = STy->getStructElementType(idx.index);
                // assert(idx.STy == Ty);
            } else assert(false);
        } else if(Ty->getTypeID() == Type::ArrayTyID) {
            if(ArrayType *arrayType = cast<ArrayType>(Ty)) {
                auto arrayElement = arrayType->getArrayElementType();
                Total += M.getDataLayout().getTypeAllocSize(arrayElement) * idx.index;
                Ty = arrayElement;
            } else assert(false);
        } else {
            errs() << "type not supported " << Ty->getTypeID() << "\n";
            assert(false && "type not supported");
        }
    }
    
    DBGS() << "irx-struct.cc: GEP Index " << Total << " bytes.\n";
    return Total * 8;
}
