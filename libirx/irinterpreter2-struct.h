#pragma once
#include <memory>
#include <iostream>
#include <unordered_map>

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
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/GlobalVariable.h>

// // Need patch: Forward declartion
// class Myinterpreter;
// // Friend class:
// friend class ::MyInterpreter
//#include <llvm/ExecutionEngine/Interpreter.h>
#include "llvm-internal/Interpreter.h"
// TODO ExecutionEngine.h needs the same patch

#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DebugInfo.h>

class IRXAllocator;

#include "irinterpreter2-mmap.h"


// logging
std::ostream& get_nulls();
//#define DBGS() llvm::dbgs()
#define DBGS() ::get_nulls()

const std::string black = "\x1B[1;30m";
const std::string red = "\x1B[1;31m";
const std::string green = "\033[1;32m";
const std::string yellow = "\033[1;33m";
const std::string blue = "\033[1;34m";
const std::string magenta = "\033[0;35m";
const std::string cyan = "\033[1;36m";
const std::string white = "\033[1;37m";
const std::string pink = "\033[1;35m";

const std::string reset = "\033[0m";
const std::string bold = "\x1B[1m";
const std::string underline  = "\x1B[4m";


// debug

inline void int3() {
#ifdef INTERPRETER_ENABLE_INT3
    asm volatile ("int3");
#endif
}


// struct

// struct Value {void* value; size_t size;};
struct FieldValue {const char * name; void* value; size_t size;}; // valueIsFieldValue = true; // the value is an Pointer to a neasted FieldValue array and the size is the length (alternative: use a std::vector)
// maybe JSON?


extern "C" struct TypedIndex { // or use std::tuple to return GEP Indices with Type Information
    llvm::Type *STy;
    size_t index;
};

extern "C" struct DITypedIndex {
    size_t index;
    llvm::DIType *DITy;
};

extern "C" struct BasicTypeInfo {
    std::string basicType;
    size_t size_in_bits;
    size_t size_in_bytes;
    std::vector<size_t> gep;
    std::vector<llvm::Type *> STys;
    // std::vector<std::string> struct_types;
    
    // maybe add this
    // size_t array_count;
    // size_t array_element_size_bytes;
};

// void printStructDebugInfo(const llvm::Module &M, const llvm::StructType *SType);
// void printStructDebugInfo(llvm::Module &M, const char *name);

std::string getFieldName(llvm::DIDerivedType* didt, int offset);

// llvm::DICompositeType* findStructDebugInfo(llvm::DebugInfoFinder &DIF, const char *name, std::unordered_map<std::string, llvm::DICompositeType*> *DICache);

std::vector<llvm::DIType *> getStructDebugInfoFields(llvm::DebugInfoFinder &DIF, const char *name);
llvm::DIType* getStructDebugInfoField(llvm::DebugInfoFinder &DIF, const char *name, const char *field);

std::vector<std::string> getExternalSymbolsInModule(llvm::Module *M);

struct IRXFlowControl {
    std::vector<std::string> stop;
    std::vector<std::string> skip;
    bool printLLVMIR = false;
    
    bool shouldStop(llvm::Function *F);
    bool shouldSkip(llvm::Function *F);
};

llvm::Module* collectLinkerScriptSections(llvm::Module *Module);

// public llvm::Interpreter
// public llvm::ExecutionEngine
class MyInterpreter: public llvm::Interpreter {
public:
    explicit MyInterpreter(std::unique_ptr<llvm::Module> M);
  
    ~MyInterpreter();

    llvm::GenericValue getConstantValue(const llvm::Constant *C);
    llvm::GenericValue getOperandValue(llvm::Value *V, llvm::ExecutionContext &SF);
    
    void StoreValueToMemory(const llvm::GenericValue &Val, llvm::GenericValue *Ptr, llvm::Type *Ty);
    void LoadValueFromMemory(llvm::GenericValue &Result, llvm::GenericValue *Ptr, llvm::Type *Ty);
    void* vvptr2vptr(uintptr_t vvptr);
    uintptr_t vptr2vvptr(void *vptr);
    
    llvm::Function* findFunctionByVPtr(void* vptr);
    llvm::Function* findFunctionByVVPtr(uintptr_t vvptr);

    void run();
    bool runInstruction(llvm::Instruction &I);
    
    std::unique_ptr<IRXAllocator> allocator;
    
    virtual char *getMemoryForGV(const llvm::GlobalVariable *GV);
    
    // Update the GlobalVariable address mapping using the provided allocator
    void updateGlobalMappings();
    bool irx_create_global(std::string name, std::string typeName);
    
    struct IRXFlowControl *flowControl;
    
    void replaceModule(std::unique_ptr<llvm::Module> M);
    
    // restore original GV's ThreadLocal value and run InitializeMemory for them
    void InitializeGlobals(std::map<std::string, bool> &isThreadLocal);
    
    // copy of regular llvm::ExecutionEngine::InitializeMemory, but will use our StoreValueToMemory for pointers
    void InitializeMemory(const llvm::Constant *Init, void *Addr);
    
    std::vector<llvm::GenericValue> getCurrentCallArgs();
    llvm::GenericValue getExitValue();
    std::vector<std::string> getCallStack();
    std::string getCurrentCallFunctionName();
    bool isRunnable();
    void putReturnValue(llvm::GenericValue Result);
    
    llvm::Instruction *runningInstruction;
    
    // Instructions
    void visitLoadInst(llvm::LoadInst &);
    void visitStoreInst(llvm::StoreInst &);
    void visitCallBase(llvm::CallBase &);
    
    
    // following code allows (clean) reuse of interpreter
    // keep track of seperate allocation that can be reseted
    // (to prevent copying the module at each step)
private:
    std::list<uintptr_t> allocations;
    uintptr_t resetLastAllocAddr;
public:
    struct irx_alloc malloc(size_t size);
    struct irx_alloc malloc(IRModuleInterpreter *irx, std::string name);
    void resetExtraAllocations();
    
    llvm::Function* getFunctionWithPtr(llvm::CallBase &call);
};

class IRModuleInterpreter {
public:
    
    llvm::LLVMContext Context;
    llvm::SMDiagnostic Err;
    
    // this is a copy and not the same Module as used by the Interpreter
    std::unique_ptr<llvm::Module> M;
    
    MyInterpreter *I = nullptr;
    // llvm::ExecutionEngine *EE = nullptr;
    llvm::DebugInfoFinder DIF;

    // std::unique_ptr<IRXAllocator> allocator;
    
    struct IRXFlowControl flowControl;
    
    IRModuleInterpreter();
    virtual ~IRModuleInterpreter();
    
    std::unordered_map<std::string, llvm::DICompositeType*> DICache;
    std::unordered_map<std::string, llvm::StructType*> STCache;
    
    std::map<llvm::GlobalVariable*, bool> isThreadLocal;
    
public:
    // Load the LLVM-IR files, link them to a single Module and create the actuall Interpreter
    bool parseIRFiles(std::vector<std::string> InputFilenames, std::string name = "CompositeModule");
    bool createInterpreter();
    // MyInterpreter* createInterpreter();
    bool runFunction(std::string FunctionName, std::vector<llvm::GenericValue> Args);
    bool resume();
    int getPointerSize();
    
    // struct
    
    struct structComponent {
    };
    struct structPrimitiveInfo: public structComponent {
        int type;
        size_t size;
    };
    struct structElementInfo: public structPrimitiveInfo {
        std::string name;
        size_t offset;
    };
    struct structComposite: public structElementInfo {
        std::vector<struct structComponent> elements;
    };
    
    void irx_struct_init(const char *name, void *m, struct FieldValue fields[], size_t numFields);
    llvm::StructType* getTypeByName(std::string name);
    llvm::StructType* getStructTypeByName(std::string name);
    size_t offsetInBitsFromPath(const char *name, std::vector<std::string> path);
    // std::unique_ptr<struct structComponent> structInfo(std::string name);
    std::vector<llvm::DIType*> structInfo(std::string name);
    struct irx_alloc structAlloc(std::string name);
    void* vvptr2vptr(uintptr_t vvptr);
    uintptr_t vptr2vvptr(void *vptr);
    size_t irx_struct_get_size(const char* name);
    void irx_struct_get_ptr(llvm::Module *M, llvm::LLVMContext &Context, std::string name, void *m, std::string field);
    void irx_struct_get(std::string name, void *m, std::vector<std::string> path, void *, size_t len);
    void irx_struct_set(const char *name, void *m, std::vector<std::string> path, void *value, size_t size);
    size_t GEP2Offset(llvm::Module &M, llvm::Type *Ty, std::list<TypedIndex> indices);
    
    std::list<struct TypedIndex> GEPIndices(std::string name, std::list<std::string> path);
    
    // debug
    struct BasicTypeInfo getStructFieldInfo(std::string name, std::vector<std::string> _needle);
    struct BasicTypeInfo getStructFieldInfo2(std::string name, std::vector<std::string> _needle);
    std::list<size_t> GEP2Member(std::string name, std::string member);
    std::list<size_t> GEP(std::string name, std::vector<std::string> needle);
    

    // global
    llvm::GenericValue irx_get_global_var(const char *name);
    void* irx_get_global_addr(const char *name);
    void irx_set_global(const char *name, void *Memory, size_t size);
    void irx_set_global(struct FieldValue FV);
    bool irx_create_global(std::string name, std::string typeName);
    bool setInitializer(std::string, std::string);
    bool hasExternal(std::string name);
    
    llvm::Constant* parseConstantString(std::string str);
    
    // Updating all GlovalVariables to be "thread local". And return there original value
    std::map<std::string, bool> fakeSetIsThreadLocalGlobals(llvm::Module *M);
    
    std::vector<std::string> getDeclaredFunction();
    bool isFunctionDeclared(std::string name);
    
    std::string findFunctionByVVPtr(uintptr_t vvptr);
    uint64_t FindFunctionNamed(std::string FunctionName);
    std::string getGlobalValueAtAddress(uintptr_t vvptr);
    void resetExtraAllocations();
    
    // analyze and control functions
    
    // set the function at which the interpreter should stop execution and return
    void stop(std::vector<std::string>);
    void skip(std::vector<std::string>);
    void setPrintLLVMIR(bool printit);
    std::vector<llvm::GenericValue> getCurrentCallArgs();
    llvm::GenericValue getExitValue();
    std::vector<std::string> getCallStack();
    std::string getCurrentCallFunctionName();
    bool isRunnable();
    void putReturnValue(llvm::GenericValue Result);

};
