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
//#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/GlobalVariable.h>
#include "llvm/AsmParser/Parser.h"
#include "llvm/AsmParser/LLParser.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/AsmParser/SlotMapping.h"

#include "irinterpreter2-struct.h"

using namespace llvm;

Constant *IRModuleInterpreter::parseConstantString(std::string str) {
    Constant *constant;
    SMDiagnostic error;
    SourceMgr srcMgr;
    struct SlotMapping slots;
    
    LLParser parser(str, srcMgr, error, M.get(), nullptr, Context, &slots);
    
    if (parser.parseStandaloneConstantValue(constant, &slots)) {
        errs() << "Could not parse constant string" << "\n";
        return nullptr;
    }
    
    return constant;
}

GenericValue IRModuleInterpreter::irx_get_global_var(const char *name) {
    GenericValue Result;
    void *VVAddr = irx_get_global_addr(name);
    void *Addr = I->allocator->vvptr2vptr((uintptr_t)VVAddr);
    const GlobalValue *GVar = I->getGlobalValueAtAddress(VVAddr);
    
    
    I->LoadValueFromMemory(Result, (GenericValue*) Addr, GVar->getValueType());
    
    return Result;
}

void* IRModuleInterpreter::irx_get_global_addr(const char *name) {
    GlobalVariable *GV = I->FindGlobalVariableNamed(name, true);
    void *VVAddr = nullptr;
    if(GV) {
        VVAddr = I->getPointerToGlobalIfAvailable(GV);
    }
    return VVAddr;
}

void IRModuleInterpreter::irx_set_global(const char *name, void *Memory, size_t size) {
    void *VVAddr = irx_get_global_addr(name);    
    void *Addr = I->allocator->vvptr2vptr((uintptr_t)VVAddr);
    
    // EE->StoreValueToMemory(*((GenericValue*)GVar), (GenericValue *)&value, GV->getValueType());
    // StoreIntToMemory(GVar, (GenericValue *)&value, GV->getValueType());
    
    memcpy(Addr, Memory, size);
}

void IRModuleInterpreter::irx_set_global(struct FieldValue FV) {
    irx_set_global(FV.name, FV.value, FV.size);
}

// bool IRModuleInterpreter::irx_alloc_global() {
//     // struct irx_alloc alloc = allocator->add(Ptr, size);
//     updateGlobalMapping("nsi_simu_time", 0);
// }

bool IRModuleInterpreter::irx_create_global(std::string name, std::string typeName) {
    // return I->irx_create_global(name, typeName);
    
    Type *Ty = nullptr;
    if(typeName.compare("i32") == 0) {
        Ty = Type::getInt32Ty(Context);
    } else if(typeName.compare("i64") == 0) {
        Ty = Type::getInt64Ty(Context);
    } else {
        Ty = StructType::getTypeByName(M->getContext(), typeName);
    }
    
    if(Ty) {
//         auto *globalVariable = new GlobalVariable(*M, Ty, false, GlobalValue::ExternalLinkage,
//                                     nullptr, name);
//         
//         globalVariable->setDSOLocal(true);
//         
//         auto globalVariable = M->getNamedGlobal(name);
//         globalVariable->setExternallyInitialized(true);
    } else {
        errs() << "Type not found: " << typeName << "\n";
    }
    
    return false;
}

bool MyInterpreter::irx_create_global(std::string name, std::string typeName) {
    StructType *Ty = StructType::getTypeByName(Modules.front()->getContext(), typeName);
    if(Ty) {
        GlobalVariable *GV = new GlobalVariable(*Modules.front(), Ty, false, GlobalValue::PrivateLinkage,
                                    nullptr, name, nullptr, GlobalValue::ThreadLocalMode::LocalExecTLSModel, 0, true);
        
        size_t size = (size_t)getDataLayout().getTypeAllocSize(Ty);
        void *Ptr = getPointerToGlobalIfAvailable(GV);
        if(Ptr) {
            struct irx_alloc alloc = allocator->add(Ptr, size);
            updateGlobalMapping(name, alloc.vvptr);
            return true;
        }
    } else {
        errs() << "Type not found: " << typeName << "\n";
    }
    
    return false;
}

bool IRModuleInterpreter::hasExternal(std::string name) {
    for(auto &e : getExternalSymbolsInModule(M.get())) {
        if(name.compare(e) == 0) {
            return true;
        }
    }
    return false;
}

bool IRModuleInterpreter::setInitializer(std::string name, std::string str) {
    Constant *constant = nullptr;
    auto globalVariable = M->getNamedGlobal(name);
    
    if(!globalVariable) {
        errs() << "Global Variable not found: " << name << "\n";
        return false;
    }
    
    if(!str.empty()) {
        constant = parseConstantString(str);
        if(!constant) {
            errs() << "Could not parse constant: " << str << "\n";
            return false;
        }
    } else {
        constant = Constant::getNullValue(globalVariable->getType());
    }
    
    globalVariable->setInitializer(constant);
    
    return true;
}
