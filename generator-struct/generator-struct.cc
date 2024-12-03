#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <unistd.h>
#include <memory>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <algorithm>
#include <chrono>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>
#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/Support/Host.h"
#include <llvm/Support/SourceMgr.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DebugInfo.h>

#define DEBUG_TYPE "generator-struct"

#include "generator-struct.h"


// TODO: FIXME: function pointer are not generated: fn: PTR = field(default_factory=lambda: PTR(Int8))

using namespace std;
using namespace llvm;

cl::opt<string> InputFilename(cl::Positional, cl::desc("<LLVM-IR input file>"), cl::value_desc("filename"), cl::Required);
cl::opt<string> OutputFilename("o", cl::desc("Python dataclass output file"), cl::value_desc("filename"), cl::init("-"));
cl::opt<bool> EnableColor("c", cl::desc("Enable color"), cl::value_desc("color"), cl::init(false));

const std::string black = "\x1B[1;30m";
const std::string red = "\x1B[1;31m";
const std::string green = "\033[1;32m";
const std::string yellow = "\033[1;33m";
const std::string reset = "\033[0m";

inline const std::string color(const std::string &c) {
  if(EnableColor) return c;
  return "";
}

string underscorify(string str) {
  size_t n;
  while((n = str.find(".")) != string::npos) {
    str.replace(n, 1, "_");
  }
  return str;
}

DIType *resolveUntil(DIType* DITy, list<dwarf::Tag> kinds) {
  if(DITy) {
    // DITy->dump();
    for(auto &k : kinds) {
      if(DITy->getTag() == k) {
        return DITy;
      }
    }

    if (auto *DIDTy = dyn_cast<DIDerivedType>(DITy)) {
      DITy = resolveUntil(DIDTy->getBaseType(), kinds);
      // if(DITy && !DITy->getName().empty()) {
        return DITy;
      // }
    }
  }
  return nullptr;
}

DIType *resolve(DIType* DITy) {
  DITy = resolveUntil(DITy, {
    dwarf::DW_TAG_base_type,
    dwarf::DW_TAG_structure_type,
    dwarf::DW_TAG_union_type,
    dwarf::DW_TAG_array_type,
    dwarf::DW_TAG_pointer_type,
    dwarf::DW_TAG_typedef,
  });
  
  
//   if(DITy) {
//     // DITy->dump();
//     switch(DITy->getTag()) {
//       case dwarf::DW_TAG_structure_type:
//       case dwarf::DW_TAG_union_type:
//         if(DITy->getName().empty) {
//           
//         }
//     }
//   }
  
  return DITy;
  
  // if(DITy) {
  //   // DITy->dump();
  //   switch(DITy->getTag()) {
  //     case dwarf::DW_TAG_base_type:
  //     case dwarf::DW_TAG_structure_type:
  //     case dwarf::DW_TAG_union_type:
  //     case dwarf::DW_TAG_array_type:
  //     case dwarf::DW_TAG_pointer_type:
  //     case dwarf::DW_TAG_typedef:
  //       return DITy;
  //       break;
  //     case dwarf::DW_TAG_member:
  //     default:
  //       if (auto *DIDTy = dyn_cast<DIDerivedType>(DITy)) {
  //         DITy = resolve(DIDTy->getBaseType());
  //         // if(DITy && !DITy->getName().empty()) {
  //           return DITy;
  //         // }
  //       }
  //   }
  // }
  return nullptr;
}

bool is_member(DIType* DITy) {
  DIScope *S = DITy;
  S->dump();
  while((S = S->getScope())) {
    S->dump();
  }
  
  return false;
}

DIBasicType* toBasicType(DIType* DITy);

void generate_struct2(struct GeneratorStruct *ctx, DIDerivedType *CTy);
void generate_struct(struct GeneratorStruct *ctx, DIType* CTy);
// void generate_struct(struct GeneratorStruct *ctx, DebugInfoFinder &DIF, Module *M, string name);
DIType* print(struct GeneratorStruct *ctx, DIType* DITy, bool start = false);

// list<DIType*> done;
list<string> done;
list<string> done2;

DIType *resolve2(struct GeneratorStruct *ctx, DIType* DITy) {
  
  auto *DITyResolved = resolve(DITy);
  
  // outs() << yellow;
  // DITy->dump();
  // outs() << reset;
  // DITyResolved->dump();
  
  // DITy->dump();
  if(DITyResolved) {
  if (auto *CTy = dyn_cast<DICompositeType>(DITy)) {
      for(auto DINode : CTy->getElements()) {
        if (auto *DITy = dyn_cast<DIType>(DINode)) {
            auto *DITyResolved = resolve(DITy);
            
            if(DITyResolved) {
              // outs() << "DICompositeType:";
              // DITy->dump();
              if (auto *CTy = dyn_cast<DIDerivedType>(DITy)) {
                  generate_struct(ctx, CTy->getBaseType());
//               switch(CTy->getTag()) {
//                 case dwarf::DW_TAG_structure_type:
//                 case dwarf::DW_TAG_union_type:
//                   if(CTy->getName().empty()) {
//                     continue;
//                   }
//                 default:
//                   break;
//               }
              }
            }
//             
//             DIType *Ty = resolve2(ctx, DITyResolved);
        }
      }
  }
    // errs() << " -2 " << DITyResolved->getName();
    switch(DITyResolved->getTag()) {
      case dwarf::DW_TAG_base_type:
          // errs() << green << DITy->getName();
          // errs() << ": " << DITyResolved->getName() << " S:" << DITyResolved->getSizeInBits() << reset;
        break;
      case dwarf::DW_TAG_typedef:
          if (auto *DIDTy = dyn_cast<DIDerivedType>(DITyResolved)) {
            // ctx->typedefs.push_back(DIDTy);
            // ctx->blocks.push_back(new Typedef(DIDTy));
            
            // print(ctx, DIDTy);
            // DIDTy->dump();
            // DIDTy->getBaseType()->dump();
            generate_struct(ctx, resolve(DIDTy->getBaseType()));
            
//             auto *DIAliaseeTy = print(ctx, DIDTy->getBaseType());
//             if(DIAliaseeTy) {
//               errs() << yellow << DIDTy->getName() << "=" << DIAliaseeTy->getName();
//               errs() << reset;
// //               if(DIAliaseeTy->getTag() == dwarf::DW_TAG_typedef) {
// //                 if (auto *DIDTy = dyn_cast<DIDerivedType>(DIAliaseeTy)) {
// //                   
// //                   
// //                   
// //                 }
// //               }
//             }
  
            // return DIDTy;
            
          }
        break;
      case dwarf::DW_TAG_union_type:
      case dwarf::DW_TAG_structure_type:
          if(DITyResolved->getName().empty()) {
            errs() << "FOUND EMPTY NAME: try to fallback to typedef name\n";
            // if(is_member(DITyResolved)) {
            //   DITyResolved->dump();
            // }
            if (auto *DIDTy = dyn_cast<DIDerivedType>(DITyResolved)) {
              // resolve2(ctx, DIDTy);
              
              // outs() << red;
              // DIDTy->dump();
              // outs() << reset;
              generate_struct(ctx, DIDTy->getBaseType());
            }
          } else {
            if (auto *DIDTy = dyn_cast<DIDerivedType>(DITyResolved)) {
            if (auto *DIDTy2 = dyn_cast<DIDerivedType>(DIDTy)) {
              // print(ctx, DIDTy);
              generate_struct(ctx, DIDTy2->getBaseType());
            }
            }
          }
          // if(true) {
          //   errs() << yellow << DITyResolved->getTag();
          //   errs() << ": " << DITyResolved->getName() << reset;
          // }
        break;
      case dwarf::DW_TAG_array_type:
          // errs() << green << "    arr1" << reset << "\n";
          if (auto *DIDTy = dyn_cast<DICompositeType>(DITyResolved)) {
            // errs() << green << "    arr2" << reset << "\n";
            if(DIDTy->getBaseType()) {
            // errs() << green << "    arr3" << reset << "\n";
              // DIDTy->getBaseType()->dump();
              // auto *PTRDITy = print(ctx, DIDTy->getBaseType());
              generate_struct(ctx, DIDTy->getBaseType());
              // if(PTRDITy && true) {
              //   size_t n = DIDTy->getSizeInBits() / PTRDITy->getSizeInBits();
              //   errs() << ": " << yellow << PTRDITy->getName() << "[" << n << "]" << reset;
              // }
            }
          }
          
        break;
      case dwarf::DW_TAG_pointer_type:
          if (auto *DIDTy = dyn_cast<DIDerivedType>(DITyResolved)) {
            if(DIDTy->getBaseType()) {
              // DIDTy->getBaseType()->dump();
              generate_struct(ctx, DIDTy->getBaseType());
//               auto *PTRDITy = print(ctx, DIDTy->getBaseType());
//               if(PTRDITy) {
//                 errs() << yellow << DITy->getName() << ": PTR " << DITyResolved->getName();
//                 errs() << "*" << PTRDITy->getName() << "";
//               }
//               
//               return PTRDITy;
            }
          }
        break;
      default:
        errs() << "UNKNOWN TYPE" << "";
        break;
    }
  }
  *ctx->Output << "\n";
  // if (auto *DIDTy = dyn_cast<DIDerivedType>(DITy)) {
  //     if(!DIDTy->getName().str().empty()) {
  //     }
  // }

  return DITyResolved;
}

// , Dataclass *dc
void generate_struct2(struct GeneratorStruct *ctx, DIType *CTy) {
    // Dataclass dc;
  
      // for(auto DINode : CTy->getElements()) {
      //   if (auto *DITy = dyn_cast<DIType>(DINode)) {
      //       auto *DITyResolved = resolve(DITy);
      //       DIType *Ty = resolve2(ctx, DITyResolved);
      //   }
      // }
  
  resolve2(ctx, CTy);

    
    // print dataclass
}


string toPythonType(DIType* DITy) {
  string type;
  
  if (auto *CTy = dyn_cast<DIBasicType>(DITy)) {
    auto k = dwarf::TypeKind(CTy->getEncoding());
    switch (k) {
      // add special cases if needed ...
      default:
        if(CTy->getName().startswith("unsigned")) {
          type = "U";
        }
        type += "Int";
        type += to_string(DITy->getSizeInBits()) + "";
    }
  }
  
  return type;
}


DIType* print(struct GeneratorStruct *ctx, DIType* DITy, bool start) {
  auto *DITyResolved = resolve(DITy);
  
//   errs() << "DONE: ";
//   for(auto &s : done) {
//     errs() << s << " ";
//   }
//   errs() << "\n";
//   
//   list<string>::iterator findIter = std::find_if(done.begin(), done.end(),
//                       [&]( const string v ){ return v.compare(DITy->getName().str()) == 0; } );
//   if(findIter != done.end()) {
//     errs() << "STOP" << "";
//     return nullptr;
//   }
  
  // done.push_back(DITy);
  

  
  // errs() << DITy->getName() << " " << start << " (PRINTER)\n";
  // DITy->dump();

  if (  DITy->getTag()  ==  dwarf::DW_TAG_union_type
        || DITy->getTag() ==  dwarf::DW_TAG_structure_type ){
  if(!DITy->getName().empty() && start) {
    *ctx->Output << color(red) << "@dataclass\n";
    *ctx->Output << "class " << DITy->getName() << "(StructPointerTrait):\n";
    *ctx->Output << "    " << "__ptr__: PTR = field(default_factory=lambda: PTR(" << DITy->getName() << "))";
    *ctx->Output << color(reset) << "\n";
    start = false;
  }
}
  
  
  if(DITyResolved) {
  if (auto *CTy = dyn_cast<DICompositeType>(DITy)) {
      
      for(auto DINode : CTy->getElements()) {
          if (auto *DITy = dyn_cast<DIType>(DINode)) {
              // DIType *Ty = resolve2(ctx, DITy);
              // errs() << "member: " << DITy->getName() << "\n";
            
            if (auto *CTy = dyn_cast<DIDerivedType>(DITy)) {
                  // resolve2(ctx, CTy->getBaseType());
              // CTy->dump();
              // print(ctx, resolve(DITy));
              print(ctx, CTy, start);
            }
          }
      }
      
      return nullptr;
  }
  
    *ctx->Output << color(black) << "    # ";
    DITyResolved->print(*ctx->Output);
    *ctx->Output << color(reset) << "\n";
    // errs() << " -2 " << DITyResolved->getName();
    // DITyResolved->dump();
    switch(DITyResolved->getTag()) {
      case dwarf::DW_TAG_base_type:
        if(start) {
          // *ctx->Output << green << "" << DITy->getName();
          // *ctx->Output << "=" << toPythonType(DITyResolved);
          // *ctx->Output << reset << "\n";
        } else {
          *ctx->Output << color(green) << "    " << DITy->getName();
          *ctx->Output << ": " << toPythonType(DITyResolved);
          *ctx->Output << " = 0" << color(reset) << "\n";
        }
        break;
      case dwarf::DW_TAG_typedef:
          if (auto *DIDTy = dyn_cast<DIDerivedType>(DITyResolved)) {
            
            auto *DIAliaseeTy = resolve(DIDTy->getBaseType());
            if(DIAliaseeTy) {
              if(!DIAliaseeTy->getName().empty()) {
                  if(!start) {
                    if(auto *BTy = toBasicType(DIAliaseeTy)) {
                      *ctx->Output << color(yellow) << "    " << DITy->getName() << ": " << toPythonType(BTy);
                      *ctx->Output << " = 0" << "\n" << color(reset);
                    } else {
                      *ctx->Output << color(yellow) << "    " << DITy->getName() << ": " << DIDTy->getName();
                      *ctx->Output << " = field(default_factory=" << DIDTy->getName() << ")\n" << color(reset);
                    }
                  } else {
                    if (auto *BTy = toBasicType(DIAliaseeTy)) {
                      *ctx->Output << color(yellow) << "" << DITy->getName();
                      *ctx->Output << "=" << toPythonType(BTy);
                      *ctx->Output << color(reset) << "\n";
                    } else {
                      *ctx->Output << color(yellow) << DIDTy->getName() << "=" << DIAliaseeTy->getName();
                      *ctx->Output << "\n" << color(reset);
                    }
                  }
              } else {
                // errs() << yellow << "start new dataclass" << reset << "\n";
                
                if(start) {
                  *ctx->Output << color(red) << "@dataclass\n";
                  *ctx->Output << color(red) << "class " << DITy->getName() << "(StructPointerTrait):" << color(reset) << "\n";
                  *ctx->Output << "    " << "__ptr__: PTR = field(default_factory=lambda: PTR(" << DITy->getName() << "))" << "\n";
                  // DIAliaseeTy->dump();
                  // generate_struct(ctx, DIAliaseeTy);
                  print(ctx, DIAliaseeTy, false);
                } else {
                    if(auto *BTy = toBasicType(DIAliaseeTy)) {
                      *ctx->Output << color(yellow) << "    " << DITy->getName() << ": " << toPythonType(BTy);
                      *ctx->Output << " = 0" << "\n" << color(reset);
                    } else {
                      *ctx->Output << color(yellow) << "    " << DITy->getName() << ": " << DIDTy->getName();
                      *ctx->Output << " = field(default_factory=" << DIDTy->getName() << ")\n" << color(reset);
                    }
                }
              }
            }
          }
        break;
      case dwarf::DW_TAG_union_type:
      case dwarf::DW_TAG_structure_type:
          // errs() << "struct name: " << DITyResolved->getName() << "\n";
          if(DITyResolved->getName().empty()) {
          //   errs() << "FOUND EMPTY NAME: try to fallback to typedef name";
            // if (auto *DIDTy = dyn_cast<DICompositeType>(DITyResolved)) {
            //   print_dataclass(DIDTy);
            // }
            if (auto *DIDTy = dyn_cast<DIDerivedType>(DITyResolved)) {
              print(ctx, DIDTy, start);
            }
            
            if (auto *CTy = dyn_cast<DICompositeType>(DITyResolved)) {
              print(ctx, CTy, start);
            }
          } else {
          //   if (auto *DIDTy = dyn_cast<DICompositeType>(DITyResolved)) {
          //     generate_struct2(ctx, DIDTy);
          //   }
            
            if (auto *BTy = toBasicType(DITyResolved)) {
              *ctx->Output << color(green) << "    " << DITy->getName();
              *ctx->Output << ": " << toPythonType(BTy);
              *ctx->Output << " = 0\n" << color(reset);
            } else {
              *ctx->Output << color(green) << "    " << DITy->getName();
              *ctx->Output << ": " << DITyResolved->getName();
              *ctx->Output << " = field(default_factory=" << DITyResolved->getName() << ")\n" << color(reset);
            }
            
            
          }
          // if(true) {
          // }
        break;
      case dwarf::DW_TAG_array_type:
          // errs() << color(green) << "    arr]" << reset << "\n";
          if (auto *DIDTy = dyn_cast<DICompositeType>(DITyResolved)) {
            if(DIDTy->getBaseType()) {
              // DIDTy->getBaseType()->dump();
              auto *PTRDITy = resolve(DIDTy->getBaseType());
              if(PTRDITy) {
                
                if(PTRDITy->getTag() == dwarf::DW_TAG_pointer_type) {
                  if (auto *DIDTy = dyn_cast<DICompositeType>(PTRDITy)) {
                    PTRDITy = resolve(DIDTy->getBaseType());
                  }
                }
                
                size_t n = DIDTy->getSizeInBits() / PTRDITy->getSizeInBits();
                *ctx->Output << color(green) << "    "<< DITy->getName() << ": ";
                if(PTRDITy->getName().compare("char") == 0) {
                  *ctx->Output << "Annotated(str, ArraySize(" << n << ")) = field(default_factory=str)" << color(reset) << "\n";
                } else {
                  *ctx->Output << "Annotated(list[" << PTRDITy->getName() << "], ArraySize(" << n << ")) = field(default_factory=lambda: [" << PTRDITy->getName() << "()] * " << n << ")" << color(reset) << "\n";
                }
              }
            }
          }
          
        break;
      case dwarf::DW_TAG_pointer_type:
          if (auto *DIDTy = dyn_cast<DIDerivedType>(DITyResolved)) {
            if(DIDTy->getBaseType()) {
              // DIDTy->getBaseType()->dump();
              auto *PTRDITy = resolve(DIDTy->getBaseType());
              if(PTRDITy) {
                // DITy->dump();
                // PTRDITy->dump();
                *ctx->Output << color(green) << "    "<< DITy->getName() << ": " << "PTR";
                *ctx->Output << " = " << "field(default_factory=lambda: PTR(" << PTRDITy->getName() << "))" << color(reset) << "\n";
                
                return PTRDITy;
              }
            }
          }
        break;
      default:
        errs() << "UNKNOWN TYPE" << "";
        break;
    }
  }
  
  
    *ctx->Output << "\n\n";
    
    return nullptr;
}

DIType* findStructDebugInfo2(DebugInfoFinder &DIF, const char *name) {
    for (auto *Ty : DIF.types()) {
        if (Ty->getName() == name) {
            if (Ty->getTag() == dwarf::DW_TAG_structure_type
              || Ty->getTag() == dwarf::DW_TAG_union_type
              || Ty->getTag() == dwarf::DW_TAG_typedef) {
                if (auto *CTy = dyn_cast<DIDerivedType>(Ty)) {
                    return CTy;
                }
                
                if (auto *CTy = dyn_cast<DICompositeType>(Ty)) {
                    return CTy;
                }
            }
        }
    }
    
    errs() << "Struct not found: " << name << "\n";
    return nullptr;
}

void print_dot(GeneratorStruct *ctx, DIType* CTy) {
  
  list<string>::iterator findIter = std::find_if(done2.begin(), done2.end(),
                      [&]( const string v ){ return v.compare(CTy->getName().str()) == 0; } );
  if(findIter != done2.end()) {
    // errs() << "STOP" << "";
    return;
  }
  
  if(!CTy->getName().empty()) {
    done2.push_back(CTy->getName().str());
  }
  
  
      *ctx->Output << "\"" << CTy << "\"[label=\"";
      *ctx->Output << dwarf::TagString(CTy->getTag()) << "\\n" << CTy->getName();
      
      // switch(CTy->getTag()) {
      //   case dwarf::DW_TAG_base_type:
      //     outs() << CTy->getName() << "\n";
      //     break;
      // //   case dwarf::DW_TAG_typedef:
      // //     outs() << DIDTy->getName() << " -> " << DIAliaseeTy->getName() << "\n";
      // //     break;
      // //   case dwarf::DW_TAG_union_type:
      // //   case dwarf::DW_TAG_structure_type:
      // //     outs() << DIDTy << " -> " << DIAliaseeTy->getName() << "\n";
      // //     break;
      // //   case dwarf::DW_TAG_array_type:
      // //     outs() << DIDTy->getName() << " -> " << DIAliaseeTy->getName() << "\n";
      // //     break;
      // //   case dwarf::DW_TAG_pointer_type:
      // //     outs() << DIDTy->getName() << " -> " << DIAliaseeTy->getName() << "\n";
      // //     break;
      //   default:
      //     outs() << "UNKNOWN TYPE" << "";
      //     break;
      // }
      
      *ctx->Output << "\"]" << "\n";
  
  if (DICompositeType *CT2y = dyn_cast<DICompositeType>(CTy)) {
    
//       outs() << "\"" << CT2y << "\"[label=\"E\n";
//       outs() << dwarf::TagString(CT2y->getTag()) << "\n" << CT2y->getName();
//       
//       outs() << "\"]" << "\n";
    
    if(CT2y->getTag() == dwarf::DW_TAG_array_type) {
    auto *DIAliaseeTy = CT2y->getBaseType();
    if(DIAliaseeTy){
    // DIAliaseeTy->dump();
    
      print_dot(ctx, DIAliaseeTy);
      // outs() << "print: " << "\n";
      

      
  *ctx->Output << "\"" << CT2y << "\" -> \"" << DIAliaseeTy << "\"\n";
  // outs() << "\"" << DIDTy << "\" -> \"" << DIAliaseeTy << "\"\n";
      
  }
    }
    
    for(auto DINode : CT2y->getElements()) {
      if (auto *DITy = dyn_cast<DIType>(DINode)) {
        print_dot(ctx, DITy);
        
        *ctx->Output << "\"" << CT2y << "\" -> \"" << DITy << "\"\n";
        
      }
    }
  }
  
  
  if (auto *DIDTy = dyn_cast<DIDerivedType>(CTy)) {
    auto *DIAliaseeTy = DIDTy->getBaseType();
    if(DIAliaseeTy){
    // DIAliaseeTy->dump();
    
      print_dot(ctx, DIAliaseeTy);
      // outs() << "print: " << "\n";
      

      
  *ctx->Output << "\"" << DIDTy << "\" -> \"" << DIAliaseeTy << "\"\n";
  // outs() << "\"" << DIDTy << "\" -> \"" << DIAliaseeTy << "\"\n";
      
  }
  }
  

}

// from -struct.cc
StructType* getTypeByName(Module *M, std::string name);

void generate_struct(struct GeneratorStruct *ctx, DIType* CTy) {
  
  if(!CTy) return;
  
  StructType *ST = getTypeByName(ctx->M, "struct." + CTy->getName().str());
  
  // errs() << "DONE: ";
  // for(auto &s : done) {
  //   errs() << s << " ";
  // }
  // errs() << "\n";
  
  list<string>::iterator findIter = std::find_if(done.begin(), done.end(),
                      [&]( const string v ){ return v.compare(CTy->getName().str()) == 0; } );
  if(findIter != done.end()) {
    // errs() << "STOP" << "";
    return;
  }
  
  if(!CTy->getName().empty()) {
    done.push_back(CTy->getName().str());
  }
  
    //outs() << "generate_strut enter pass 1:" << "";
    //CTy->dump();if(ST) ST->dump();
  
  
    generate_struct2(ctx, CTy);
    //outs() << "generate_strut exit pass 1:" << "";
    //CTy->dump(); if(ST) ST->dump();
    
  if(!CTy->getName().empty()) {
    
    *ctx->Output << color(black);
    *ctx->Output << "\"\"\"" << "\n";
    *ctx->Output << "digraph{ " << "\n";
    print_dot(ctx, CTy);
    *ctx->Output << "} " << "\n";
    *ctx->Output << "\"\"\"" << "\n";
    
    *ctx->Output << "# ";
    // CTy->dump();
    CTy->print(*ctx->Output);
    *ctx->Output << "\n";
    
    if(ST) {
      *ctx->Output << "# ";
      // ST->dump();
      ST->print(*ctx->Output);
      *ctx->Output << "\n";
    }
    
    *ctx->Output << color(reset);
    
    // outs() << "generate_strut enter pass 2:" << "";
    // CTy->dump(); if(ST) ST->dump();
    print(ctx, CTy, true);
    // outs() << "generate_strut exit pass 2:" << "";
    // CTy->dump(); if(ST) ST->dump();
    
  
  
    // errs() << "    pass\n\n\n";
  }
  // outs() << "generate_strut enter 3:" << "";
  // CTy->dump();
}

void generate_struct(struct GeneratorStruct *ctx, DebugInfoFinder &DIF, string name) {
    // errs() << red << name << reset << "\n";
    DIType* CTy = findStructDebugInfo2(DIF, name.c_str());
    
    if(!CTy) return;
    
    // auto dc = new Dataclass(CTy);
    // ctx->blocks.push_back(dc);
    
    // generate_struct2(ctx, dc, CTy);
    
    generate_struct(ctx, CTy);
}

void dumpTypedef(DIDerivedType *DIDTy) {
//   auto *DIAliaseeTy = resolve(DIDTy->getBaseType());
//   if(DIAliaseeTy) {
//     errs() << DIDTy->getName() << "=" << DIAliaseeTy->getName() << "\n";
//     
//     if(DIAliaseeTy->getTag() == dwarf::DW_TAG_typedef) {
//       if (auto *DIDTy = dyn_cast<DIDerivedType>(DIAliaseeTy)) {
//         dumpTypedef(DIDTy);
//       }
//     }
//   }
}

void dumpTypedefs(struct GeneratorStruct *ctx) {
  // for(auto *DIDTy : ctx->typedefs) {
  //   dumpTypedef(DIDTy);
  // }
}

bool allow_list(std::string name) {
  std::list<const char *> allowed = {
    "_dnode",
    "k_sem",
    "k_timer",
    "_timeout",
    "k_timeout_t",
    "k_spinlock",
    "_thread_base",
    "_wait_q_t",
    "k_mutex",
    "k_msgq",
    "k_thread",
    "z_kernel",
    "_cpu",
    "_ready_q",
    "k_work_q",
    "k_fifo",
    "k_queue",
    "k_work",
  };
  
  for(auto &x : allowed) {
    if(name.compare(x) == 0) {
      errs() << "OK" << "\n";
      return true;
    }
  }
  
  return false;
}

bool block_list(std::string name) {
  std::list<const char *> blocked = {
    "log_msg",
    "log_msg_ptr",
    "mpsc_pbuf_buffer_config.160",
    "mpsc_pbuf_generic",
  };
  
  for(auto &x : blocked) {
    if(name.compare(x) == 0) {
      return true;
    }
  }
  
  return false;
}

void generate(Module *M, raw_ostream *Output) {
  DebugInfoFinder DIF;
  DIF.processModule(*M);
  
  struct GeneratorStruct ctx{M, Output};
  
  auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  
  *ctx.Output << 
R"EOF("""
This file was auto generated by generator-struct
)EOF" << std::ctime(&now) << R"EOF("""

from __future__ import annotations
from dataclasses import dataclass, field
from typing import Annotated, Optional, Union, TypedDict
import ctypes

# from irx.proxy import PTR

UInt8  = Annotated[int, ctypes.c_uint8]
UInt16 = Annotated[int, ctypes.c_uint16]
UInt32 = Annotated[int, ctypes.c_uint32]
UInt64 = Annotated[int, ctypes.c_uint64]
Int8   = Annotated[int, ctypes.c_int8]
Int16  = Annotated[int, ctypes.c_int16]
Int32  = Annotated[int, ctypes.c_int32]
Int64  = Annotated[int, ctypes.c_int64]
char   = Annotated[int, ctypes.c_int8]

@dataclass
class VOID:
    pass

@dataclass
class ArraySize:
    value: int

@dataclass
class StructPointerTrait:
    @property
    def ptr(self) -> int:
        return self.__ptr__.ptr
    
    @property
    def vvptr(self) -> hex:
        return hex(self.__ptr__.ptr) if self.__ptr__.ptr != None else hex(0)
    
    @ptr.setter
    def ptr(self, value: int):
        self.__ptr__.ptr = value


)EOF";
  
  // generate_struct(&ctx, DIF, "z_kernel");
  // generate_struct(&ctx, DIF, "_cpu");
  // generate_struct(&ctx, DIF, "sys_dnode_t");
  // generate_struct(&ctx, DIF, "sys_dlist_t");
  // generate_struct(&ctx, DIF, "_dnode");
  // generate_struct(&ctx, DIF, "_wait_q_t");
  
  for(StructType *ST : M->getIdentifiedStructTypes()) {
    auto name = ST->getName().split(".");
    
    errs() << name.first << ": " << underscorify(name.second.str()) << "\n";
    
    // if(!allow_list(name.second.str())) {
    //   continue;
    // }
    
    if(block_list(name.second.str())) {
      continue;
    }
    
    generate_struct(&ctx, DIF, name.second.str());
  }
  
  ctx.Output->flush();
}

int main(int argc, char* argv[]) {
    error_code EC;
    cl::ParseCommandLineOptions(argc, argv, "Python dataclass struct generator from LLVM-IR files\n");
        
    raw_ostream *Output = &outs();
    
    if (OutputFilename != "-") {
      raw_fd_ostream *out = new raw_fd_ostream(OutputFilename.c_str(), EC);
      
      if (out->has_error()) {
        errs() << "generator-struct: Could not create output file" << "\n";
        out->close();
        delete out;
        return -1;
      }
      
      Output = out;
    }
    
    LLVMContext Context;
    SMDiagnostic Err;
    auto InputModule = llvm::parseIRFile(InputFilename, Err, Context);
    
    generate(InputModule.get(), Output);

    if(raw_fd_ostream *O = dynamic_cast<raw_fd_ostream*>(Output)) {
      O->close();
      delete O;
    }
    
    return 0;
}
