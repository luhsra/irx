#include <iostream>
#include <unordered_map>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DebugInfo.h>
#include "irinterpreter2-struct.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/Metadata.h"

using namespace llvm;

DICompositeType* findStructDebugInfo(DebugInfoFinder &DIF, const char *name, std::unordered_map<std::string, llvm::DICompositeType*> *DICache) {
    if(DICache) {
        auto it = DICache->find(name);
        if (it != DICache->end()) {
            return it->second;
        }
    }

    for (auto *Ty : DIF.types()) {
        if (Ty->getName() == name) {
            if (Ty->getTag() == dwarf::DW_TAG_structure_type) {
                if (auto *CTy = dyn_cast<DICompositeType>(Ty)) {
                    (*DICache)[name] = CTy;
                    return CTy;
                }
            }
            // else if(Ty->getTag() == dwarf::DW_TAG_typedef) {
            //     if (auto *DIDTy = dyn_cast<DIDerivedType>(Ty)) {
            //         if (auto *CTy = dyn_cast<DICompositeType>(DIDTy->getBaseType())) {
            //             return CTy;
            //         }
            //     }
            // }
        }
    }
    
    errs() << "Struct not found: " << name << "\n";
    if(DICache) {
        (*DICache)[name] = nullptr;
    }
    return nullptr;
}

std::vector<llvm::DIType *> getStructDebugInfoFields(DebugInfoFinder &DIF, const char *name) {
    std::vector<llvm::DIType *> fields;
    DICompositeType* CTy = findStructDebugInfo(DIF, name, nullptr);
    
    if(CTy == nullptr) {
        errs() << "Struct Not Found: " << name << "\n";
        return fields;
    }
    
    auto elements = CTy->getElements();
    for(auto el : elements) {
        if (auto *ECTy = dyn_cast<DIDerivedType>(el)) {
            
            if (auto *DITy = dyn_cast<DIType>(ECTy)) {
                fields.push_back(DITy);
            }
        }
    }
    return fields;
}

DIType* getStructDebugInfoField(DebugInfoFinder &DIF, const char *name, const char *field) {
    auto elements = getStructDebugInfoFields(DIF, name);

    for(auto DITy : elements) {
        if(DITy->getName() == field) {
            return DITy;
        }
    }
    
    errs() << "Field in struct not found: " << name << "." << field << "\n";
    return nullptr;
}

bool process_composite_struct(BasicTypeInfo *info, DICompositeType* CTy, std::list<std::string> path, std::list<std::string> needle);

bool process_element(BasicTypeInfo *info, DIType* DITy, std::list<std::string> path, std::list<std::string> needle) {
    bool found = std::equal(needle.begin(), needle.end(), path.begin(), path.end());
    
    if(found) {
        DBGS() << "process_element path: ";
        for(auto i : path) DBGS() << i << ".";
        DBGS() << " found: " << found;
        DBGS() << "\n";
    }
    
    if (auto *DIBTy = dyn_cast<DIBasicType>(DITy)) {
        if(found) {
            info->basicType = DIBTy->getName().str();
            info->size_in_bits = DIBTy->getSizeInBits();
            assert((DIBTy->getSizeInBits() & 0x07) == 0 && "No support for bitfields");
            info->size_in_bytes = DIBTy->getSizeInBits() / 8;
            DBGS() << "  struct Info DIBasicType: " << DIBTy->getName().str() << " S:" << DIBTy->getSizeInBits() << "\n";
        }
    }
    if (auto *CTy = dyn_cast<DICompositeType>(DITy)) {
        if(CTy->getTag() == dwarf::DW_TAG_structure_type) {
            // DBGS() << "  struct Info DW_TAG_structure_type: " << CTy->getName() << " S:" << CTy->getSizeInBits() << "\n";
            //path.push_back(CTy->getName().str());
            return process_composite_struct(info, CTy, path, needle);
        }
        else if(CTy->getTag() == dwarf::DW_TAG_union_type) {
            // outs() << "  struct Info DW_TAG_union_type: " << CTy->getName() << " S:" << CTy->getSizeInBits() << "\n";
            return process_composite_struct(info, CTy, path, needle);
        }
        else if(CTy->getTag() == dwarf::DW_TAG_array_type) {
            // DBGS() << "  struct Info DW_TAG_array_type: " << CTy->getName() << " S:" << CTy->getSizeInBits() << "\n";
            DIType *arrayDITy = CTy->getBaseType();
            DINodeArray DIArrayElements = CTy->getElements();
            assert(DIArrayElements.size() == 1 && "Not supported");
            // auto subrange = DIArrayElements.get();
            size_t arraySize = CTy->getSizeInBits() / arrayDITy->getSizeInBits();

            bool found_new = false;

            // for(int i = 0; i < arraySize; ++i) {
            //     std::list<std::string> array_path(path.begin(), path.end());
            //     array_path.push_back(std::to_string(i));
            //     found_new = process_element(info, arrayDITy, array_path, needle);
            //     if(found_new) {
            //         break;
            //     }
            // }

            
            found_new = process_composite_struct(info, CTy, path, needle);
            // if (auto *arrCTy = dyn_cast<DICompositeType>(arrayDITy)) {
            //     found_new = process_composite_struct(info, CTy, path, needle);
            // } else if (auto *arrDIBTy = dyn_cast<DIBasicType>(arrayDITy)) {
            //     found_new = process_element(info, arrayDITy, path, needle);
            // }
            
            if(found) {
                info->size_in_bits *= arraySize;
                info->size_in_bytes *= arraySize;
            }
            
            
            // info->basicType = arrayDITy->getName().str();
            // info->basicType = "array." + arrayDITy->getName().str();
            // info->size_in_bits = arrayDITy->getSizeInBits() * arraySize;
            // info->size_in_bytes = arrayDITy->getSizeInBits() / 8 * arraySize;

            // return found;
            return found_new;
        } else {
            assert(false && "process_element: DICompositeType Type not handeled");
        }
    }
    if (auto *DIDTy = dyn_cast<DIDerivedType>(DITy)) {
        if(DIDTy->getTag() == dwarf::DW_TAG_pointer_type) {
            if(found) {
                info->basicType = "PTR";
                size_t ptr_size_bits = DIDTy->getSizeInBits();
                
                info->size_in_bits = ptr_size_bits;
                assert((ptr_size_bits & 0x07) == 0 && "No support for bitfields");
                info->size_in_bytes = ptr_size_bits / 8;
            }
            // DBGS() << yellow << "  struct Info DW_TAG_pointer_type bytes: " << info->size_in_bytes << reset << "\n";
        }
        if(DIDTy->getTag() == dwarf::DW_TAG_typedef) {
            // outs() << "  struct Info DW_TAG_typedef: " << DIDTy->getName() << " S:" << DIDTy->getSizeInBits() << "\n";
            DIType *typedefDITy = DIDTy->getBaseType();
            return process_element(info, typedefDITy, path, needle);
        }
    }
    
    return found;
}

// How to access union?
// llvm will only put one element into the union

// or create a struct that return all infos to struct and elements, stop at pointers struct{name, vec<info> elements}
// TODO: rewrite this part to match the pyx implimentation
// rewrite: to search interrative the next: GetInfoOfField(StructType, const char *field); (only valid for non composited types)
bool process_composite_struct(BasicTypeInfo *info, DICompositeType* CTy, std::list<std::string> path, std::list<std::string> needle) {
    size_t index = 0;
    
    for(auto DINode : CTy->getElements()) {
        if (auto *DITy = dyn_cast<DIType>(DINode)) {
            // DBGS() << "struct Info " << DITy->getName() << "\n";
            
            if (auto *DIDTy = dyn_cast<DIDerivedType>(DITy)) {
                
                // if(path.front() == DIDTy->getName()) {
                //     path.pop_front();
                // }
                
                if(!DIDTy->getName().str().empty()) // TODO: maybe guess a name
                    path.push_back(DIDTy->getName().str());

                // DBGS() << " struct Info DIDerivedType: " << DIDTy->getName() << " S:" << DIDTy->getSizeInBits() << "\n";
                DIType *baseDITy = DIDTy->getBaseType();
                bool found = process_element(info, baseDITy, path, needle);

                if(found) {
                    info->gep.push_back(index);
                    return true;
                }

                bool is_union = false;
                // is this generally true for all unions?
                // => investigation by examples: the largest type will be the only element in that union
                if((CTy->getTag() == dwarf::DW_TAG_union_type)) {
                    is_union = true;
                }
                
                // always force the first element on unions. - notice: this is not the corrrect LLVM way
                if(!is_union) {
                    ++index;
                }
                
                if(!DIDTy->getName().str().empty())
                    path.pop_back();
            }
        }
        // ++index;
    }

    if(CTy->getTag() == dwarf::DW_TAG_array_type) {
        // DBGS() << "  struct Info DW_TAG_array_type: " << CTy->getName() << " S:" << CTy->getSizeInBits() << "\n";
        DIType *arrayDITy = CTy->getBaseType();
        DINodeArray DIArrayElements = CTy->getElements();
        assert(DIArrayElements.size() == 1 && "Not supported");
        // auto subrange = DIArrayElements.get();
        size_t arraySize = CTy->getSizeInBits() / arrayDITy->getSizeInBits();
        // DBGS() << "struct ARRAY FOUND size: " << arraySize << "\n";
        
        // TODO: add support for direct array "types" without accessing the undelining base type
        // currently it will iterate into the base elements of the array...
        
        bool found = process_element(info, arrayDITy, path, needle);
        if(found) {
            // info->size_in_bits *= arraySize;
            // info->size_in_bytes *= arraySize;
            return true;
        }

        for(size_t i = 0; i < arraySize; ++i) {
            // std::list<std::string> array_path(path.begin(), path.end());
            path.push_back(std::to_string(i));
            bool found_new = process_element(info, arrayDITy, path, needle);
            if(found_new) {
                info->gep.push_back(i);
                return true;
            }
            path.pop_back();
        }
    }
    
    return false;
}

template<typename T>
inline void print_vec(const char* prefix, T vec) {
    DBGS() << prefix << ": ";
    for(auto i : vec) {
        DBGS() << i << " ";
    }
    DBGS() << "\n";
}

struct BasicTypeInfo IRModuleInterpreter::getStructFieldInfo(std::string name, std::vector<std::string> _needle) {
    std::list<std::string> needle(_needle.begin(), _needle.end());
    BasicTypeInfo info = {"unknown", ~0U, ~0U, {}, {}};
    DICompositeType* CTy = findStructDebugInfo(DIF, name.c_str(), &DICache);
    bool found = process_composite_struct(&info, CTy, {}, needle);
    if(found) {
        info.gep.push_back(0); // the first 0 for the GEP OP
        // info.struct_types.push_back(CTy->getName().str());
        std::reverse(info.gep.begin(), info.gep.end());
        // std::reverse(info.struct_types.begin(), info.struct_types.end());

        // print_vec("getStructFieldInfo GEP", info.gep);
        // print_vec("struct_types", info.struct_types);
    } else {
        errs() << "getStructFieldInfo: Not found ";
        errs() << "name: " << name << ".";
        for(auto &s : _needle) errs() << s << ".";
        errs() << "\n";
    }

    return info;
};

DIBasicType* toBasicType(DIType* DITy) {
  while(DITy) {
      // DBGS() << "toBasicType 1: " << DITy << "\n";
    // DITy->dump();
    if(DITy->getTag() == dwarf::DW_TAG_structure_type) break;
    if(DITy->getTag() == dwarf::DW_TAG_union_type) break;
    if(DITy->getTag() == dwarf::DW_TAG_pointer_type) break;
    
    if(auto *DTy = dyn_cast<DIDerivedType>(DITy)) {
      DITy = DTy->getBaseType();
    }
      // DBGS() << "toBasicType 2: " << DITy << "\n";
    
    if(DITy) {
      if(auto *BTy = dyn_cast<DIBasicType>(DITy)) {
        return BTy;
      } else {
          // DBGS() << "toBasicType 3: " << DITy << "\n";
        if(DITy->getTag() == dwarf::DW_TAG_array_type) {
          // DBGS() << "toBasicType 4: " << DITy << "\n";
            if(auto *DICTy = dyn_cast<DICompositeType>(DITy)) {
          // DBGS() << "toBasicType 5: " << DITy << "\n";
                DITy = DICTy->getBaseType();
            }
        } else {
          // DBGS() << "toBasicType 6: " << DITy << "\n";
            DITy = dyn_cast<DIDerivedType>(DITy);
          // DBGS() << "toBasicType 7: " << DITy << "\n";
        }
      }
    }
  }
  
  return nullptr;
}

Type* DIBasicTypetoLLVMType(llvm::LLVMContext *Context, DIBasicType* BDITy) {
    Type *Ty = nullptr;

    auto k = dwarf::TypeKind(BDITy->getEncoding());
    switch (k) {
        case llvm::dwarf::DW_ATE_float:
            return llvm::Type::getFloatTy(*Context);
        default:
            switch(BDITy->getSizeInBits()) {
                case  8: return llvm::Type::getInt8Ty(*Context);
                case 16: return llvm::Type::getInt16Ty(*Context);
                case 32: return llvm::Type::getInt32Ty(*Context);
                case 64: return llvm::Type::getInt64Ty(*Context);
            }
    }
    
    return Ty;
}

DIType* resolveDerivedTypeBaseType(DIType* DIMemberTy);

std::string DIType2StringType(DIType *DITy) {
    if(DITy) {
        if(DITy->getTag() == dwarf::DW_TAG_pointer_type) return "PTR";
        
        // DBGS() << "basicType 1: " << DITy << "\n";
        DIBasicType *DIBTy = toBasicType(DITy);
        // DBGS() << "basicType 2: " << DIBTy << "\n";
        if(DIBTy) {
            // DIBTy->dump();
            return DIBTy->getName().str();
        }
    }
    return "unknown";
}

llvm::Type* DIType2Type(IRModuleInterpreter *irm, DIType *DITy) {
    // DBGS() << "DIType2Type getStructTypeByName: " << DITy->getName().str() << "\n";
    
    DITy = resolveDerivedTypeBaseType(DITy);
    
    DIBasicType *DIBTy = toBasicType(DITy);
    
    if(DIBTy) {
        Type *BTy = DIBasicTypetoLLVMType(&irm->Context, DIBTy);
        if(BTy) {
            return BTy;
        }
    }
    
    if(auto *CTy = dyn_cast<DICompositeType>(DITy)) {
        if(DITy->getName().empty()) {
            return nullptr;
        }
        
        return irm->getStructTypeByName(DITy->getName().str());
    }
    
    return nullptr;
}

llvm::Type* DIType2TypeWrapInArrayIfNeeded(IRModuleInterpreter *irm, DIType *DITy) {
    // could possible be implemented in a more elegant way...
    
    Type *Ty = DIType2Type(irm, DITy);
    
    DITy = resolveDerivedTypeBaseType(DITy);
    if(DITy->getTag() == dwarf::DW_TAG_array_type) {
        size_t num = 0; // TODO get number of elements from subtype
        Type *ArrayTy = ArrayType::get(Ty, num);
        
        return ArrayTy;
    }
    
    return Ty;
}

struct TypedIndex DITypedIndex2TypedIndex(IRModuleInterpreter *irm, struct DITypedIndex DITI) {
    Type *Ty = nullptr;
    // DBGS() << green;
    // DBGS() << "DITypedIndex2TypedIndex: ";
    // DITI.DITy->dump();
    // DBGS() << reset << "\n";
    // Ty = DIType2TypeWrapInArrayIfNeeded(irm, DITI.DITy);
    // DBGS() << "DITypedIndex2TypedIndex: ";
    // if (Ty) Ty->dump();
    // DBGS() << "\n";
    return {Ty, DITI.index};
}

std::list<struct TypedIndex> DITypedIndex2TypedIndex(IRModuleInterpreter *irm, std::list<struct DITypedIndex> DIGEP) {
    std::list<struct TypedIndex> GEP;
    for(auto &gep : DIGEP) {
        GEP.push_back(DITypedIndex2TypedIndex(irm, gep));
    }
    
    return GEP;
}

size_t getDIFieldSize(DIType *DITy) {
    size_t size_in_bits = 0;
    
    if(auto *DIBTy = dyn_cast<DIBasicType>(DITy)) {
        size_in_bits = DIBTy->getSizeInBits();
    } else if(auto *DIDTy = dyn_cast<DIDerivedType>(DITy)) {
        size_in_bits = DIDTy->getSizeInBits();
    } else {
        assert(false);
    }

    assert((size_in_bits & 0x07) == 0 && "No support for bitfields");
    
    return size_in_bits;
}

std::list<struct DITypedIndex> GEP(DICompositeType* CTy, std::vector<std::string> needle);

struct BasicTypeInfo IRModuleInterpreter::getStructFieldInfo2(std::string name, std::vector<std::string> _needle) {
    size_t size = 0;
    DIType *DITy = nullptr;
    BasicTypeInfo info = {DIType2StringType(nullptr), 0, 0, {}, {}};
    
    DICompositeType* CTy = findStructDebugInfo(DIF, name.c_str(), &DICache);
    
    // CTy->dump();
    
    // print_vec("getStructFieldInfo2 GEP _needle: ", _needle);
    
    auto DIGEP = ::GEP(CTy, _needle);

    // DBGS() << "DIGEP" << ": ";
    // for(auto i : DIGEP) {
    //     DBGS() << i.index << " ";
    // }
    // DBGS() << "\n";
    
    // Type *Ty = DIType2Type(this, CTy);
    
    if(DIGEP.size() > 0) {
        auto GEP = DITypedIndex2TypedIndex(this, DIGEP);
        
        // offset = GEP2Offset(*M, Ty, GEP);
        // DBGS() << "getStructFieldInfo2 Ty" << ": ";
        // if(GEP.back().STy) {
        //     GEP.back().STy->dump();
        //     size = M->getDataLayout().getTypeAllocSize(GEP.back().STy);
        // }
        
        DITy = DIGEP.back().DITy;
        
        size = getDIFieldSize(DITy);
        
        info.basicType = DIType2StringType(resolveDerivedTypeBaseType(DITy));
        // DITy->dump();
        // DBGS() << "info.basicType: " << info.basicType << "\n";
        info.size_in_bits = size;
        info.size_in_bytes = size / 8;
        
        for(auto &in : GEP) {
            info.gep.push_back(in.index);
            info.STys.push_back(in.STy);
        }
    }
    
    // print_vec("getStructFieldInfo2 GEP", info.gep);
    
    return info;
};


struct GEPWithType {
    std::list<size_t> GEP;
    DIType *Ty;
};

DIType* resolveDerivedTypeBaseType(DIType* DIMemberTy) {
    if(DIMemberTy) {
        // DIMemberTy->dump();
        
        if(auto *DIDTy = dyn_cast<DIDerivedType>(DIMemberTy)) {
            DIMemberTy = DIDTy->getBaseType();
        }
        
        while(DIMemberTy->getTag() == dwarf::DW_TAG_typedef) {
            if(auto *DIDTy = dyn_cast<DIDerivedType>(DIMemberTy)) {
                DIMemberTy = DIDTy->getBaseType();
            }
        }
    }
    // DIMemberTy->dump();
    return DIMemberTy;
}

// struct GEPWithType GEP2MemberInfo(DICompositeType* CTy, std::string member) {
std::list<struct DITypedIndex> GEP2MemberInfo(DICompositeType* CTy, std::string member) {
    // DBGS() << "GEP2MemberInfo: GEP member: " << member << "\n";
    std::list<struct DITypedIndex> indices = {};
    size_t index = 0;
    if(CTy->getTag() == dwarf::DW_TAG_array_type) {
        int idx = std::stoi(member);
        // CTy->getElements()->DISubrange->getCount() maybe add a boundry check
        if(idx >= 0) {
            indices.push_back({static_cast<size_t>(idx), CTy->getBaseType()});
            // indices.push_back({static_cast<size_t>(idx), CTy});
            return indices;
        }
    } else {
        for(auto DINode : CTy->getElements()) {
            // DBGS() << yellow;
            // DINode->dump();
            // DBGS() << reset;
            if (auto *DITy = dyn_cast<DIType>(DINode)) {
                if (auto *DIDTy = dyn_cast<DIDerivedType>(DITy)) {
                    if(DIDTy->getName().str().empty()) {
                        // enter all unnamed DICompositeType types
                        if(auto *CTy = dyn_cast<DICompositeType>(DIDTy->getBaseType())) {
                            
                            // DBGS() << "GEP2MemberInfo: GEP unnamed union: " << index  << " " << member << "\n";
                            // CTy->dump();
                            
                            auto sub = GEP2MemberInfo(CTy, member);
                            
                            if(sub.size() > 0) { // we found it in an unnamed composition type
                                // DBGS() << green << "GEP2MemberInfo: GEP 2: " << index << " " << member << reset << "\n";
                                // CTy->dump();
                                
                                indices.push_back({index, CTy});
                                indices.insert(indices.end(), sub.begin(), sub.end());
                                return indices;
                            }
                        }
                    } else if(DIDTy->getName().compare(member) == 0) {
                        // DBGS() << green << "GEP2MemberInfo: GEP: " << index << " " << member << reset << "\n";
                        // DIDTy->dump();
                        indices.push_back({index, DIDTy});
                        return indices;
                    }
                }
            }
            
            // always force the first element on unions. - notice: this is not the corrrect LLVM way
            if(CTy->getTag() != dwarf::DW_TAG_union_type) {
                index++;
            }
        }
    }
    return {};
}

std::list<size_t> GEP2Member(DICompositeType* CTy, std::string member) {
    std::list<size_t> indices;
    auto gepInfo = GEP2MemberInfo(CTy, member);
    for(auto &info : gepInfo) {
        indices.push_back(info.index);
    }
    return indices;
}

DIType* getMemberAt(DICompositeType* CTy, size_t index) {
    DIType* DITy = nullptr;
    size_t i = 0;
    
    if(CTy->getTag() == dwarf::DW_TAG_array_type) {
        return CTy->getBaseType();
    } else {
        for(auto DINode : CTy->getElements()) {
            if (auto *DITy = dyn_cast<DIType>(DINode)) {
                if(i == index) {
                    if (auto *DIDTy = dyn_cast<DIDerivedType>(DITy)) {
                        if(auto *NewCTy = dyn_cast<DICompositeType>(DIDTy->getBaseType())) {
                            return NewCTy;
                        }
                    }
                }
            }
            ++i;
        }
    }
    
    return DITy;
}

DIType* getMemberAt(DICompositeType* CTy, std::vector<size_t> gep) {
    DIType* DITy = nullptr;
    
    for(size_t i : gep) {
        if(CTy) {
            // CTy->dump();
            DIType* NextDITy = getMemberAt(CTy, i);
            
            CTy = nullptr;
            if(NextDITy) {
                DITy = NextDITy;
                if(auto *NewCTy = dyn_cast<DICompositeType>(NextDITy)) {
                    CTy = NewCTy;
                }
            }
        } else {
            return nullptr;
        }
    }
    
    return DITy;
}

std::list<struct DITypedIndex> GEP(DICompositeType* CTy, std::vector<std::string> needle) {
    
    // int idx = std::stoi(needle.front()); // add support for arrays
    
    std::list<struct DITypedIndex> indices = {{0, CTy}};
    
    for(auto &member : needle) {
        // DBGS() << "::GEP CTy " << CTy << "\n";
        if(!CTy) {
            errs() << red << "::GEP: returning empty GEP, since CTy is null " << reset << "\n";
            return {};
        }
        // CTy->dump();
        
        // DBGS() << "::GEP member " << member << "\n";
        auto sub = GEP2MemberInfo(CTy, member);
        CTy = nullptr;
        
        // DBGS() << "::sub size :" << sub.size() << "\n";
        
        if(sub.size() > 0) {
            indices.insert(indices.end(), sub.begin(), sub.end());
            auto *DIBTy = resolveDerivedTypeBaseType(sub.back().DITy);
            // DIBTy->dump();
            if (auto *NewCTy = dyn_cast<DICompositeType>(DIBTy)) {
                CTy = NewCTy;
            }
            
//             std::list<size_t> indices;
//             for(auto &info : sub) {
//                 indices.push_back(info.index);
//             }
//             
//             DIType *DITy = getMemberAt(CTy, std::vector<size_t> (indices.begin(), indices.end()));
//             if(DITy) {
//                 if (auto *NewCTy = dyn_cast<DICompositeType>(DITy)) {
//                     CTy = NewCTy;
//                 }
//             }
        } else {
            for(auto &member : needle) {
                errs() << member << ".";
            }
            errs() << "\n";
            errs() << red << "::GEP: returning empty GEP " << reset << "\n";
            return {};
        }
    }
    
    // DBGS() << blue << "::GEP: indices.size(): " << indices.size() << reset << "\n";
    
    return indices;
}


// DIType* MemberOf(DICompositeType* CTy, std::vector<std::string> needle) {
//     std::list<size_t> gep = GEP(CTy, needle);
//     return getMemberAt(CTy, std::vector<size_t> (gep.begin(), gep.end()));
// }
// 
// struct BasicTypeInfo InfoOf(DICompositeType* CTy, std::vector<std::string> needle) {
//     return M.Name M.Type M.GEP
// }

std::list<size_t> IRModuleInterpreter::GEP2Member(std::string name, std::string member) {
    DICompositeType* CTy = findStructDebugInfo(DIF, name.c_str(), &DICache);
    return ::GEP2Member(CTy, member);
}

std::list<size_t> IRModuleInterpreter::GEP(std::string name, std::vector<std::string> needle) {
    DICompositeType* CTy = findStructDebugInfo(DIF, name.c_str(), &DICache);
    std::list<size_t> indices;
    auto gepInfo = ::GEP(CTy, needle);
    for(auto &info : gepInfo) {
        indices.push_back(info.index);
    }
    
    print_vec("GEP GEP", indices);
    
    return indices;
}
