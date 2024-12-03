#include <cstdint>
#include <unistd.h>
#include <iostream>

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
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DebugInfo.h>
#include "irinterpreter2-struct.h"
#include "llvm/BinaryFormat/Dwarf.h"
#include <llvm/Linker/Linker.h>
#include <sys/personality.h>
    
using namespace llvm;

static cl::list<std::string> InputFilenames(cl::Positional, cl::desc("<Input files>"), cl::OneOrMore);
static cl::opt<std::string> FunctionName("func", cl::desc("Function to run"), cl::value_desc("function name"));

int main(int argc, char **argv) {
    InitLLVM X(argc, argv);
    cl::ParseCommandLineOptions(argc, argv, "LLVM Interpreter with Python helpers\n");

    IRModuleInterpreter irx;
    
    irx.parseIRFiles(InputFilenames);
    irx.createInterpreter();
    
    // getStructDebugInfoFields(*irx.M, threadType);
    // for(auto DITy : getStructDebugInfoFields(*irx.M, "thread")) {
    //     outs() << "getStructDebugInfoFields thread." << DITy->getName() << "\n";
    // }
    
    
    // BEGIN init struct
    
    int id = 42;
    unsigned long long idUL = 42;
    bool running = true;
    int cpu = 15;
    char fname[16] = "t4";
    void* ptr = nullptr;
    
    // TODO fill in neasted struct
    
    // struct FieldValue fields[] = {
    //     {".id", &id, sizeof(id)},
    //     {".running", &running, 1},
    //     {".cpu", &cpu, sizeof(cpu)},
    //     {".name", fname, sizeof(fname)},
    //     {".ptr", &ptr, sizeof(ptr)},
    // };
    // or std::vector<std::string> fields {".", "id"}
    
//     void *base = structAlloc(&(*irx.M), irx.Context, "struct._thread_base");
//     
//     struct FieldValue base_fields[] = {
//         {"id", &id, sizeof(id)},
//     };
//     
//     irx_struct_init((*irx.M), "_thread_base", base, base_fields, 1);
    
    struct FieldValue fields[] = {
        // {"base", &base, irx_struct_get_size(&(*irx.M), irx.Context, "struct._thread_base")},
        // {"base", base_fields},
        {"id", &id, sizeof(id)},
        {"running", &running, 1},
        {"cpu", &cpu, sizeof(cpu)},
        {"name", fname, sizeof(fname)},
        {"ptr", &ptr, sizeof(ptr)},
    };
    
    size_t numFields = sizeof(fields) / sizeof(struct FieldValue);
    
    // auto DITys = irx.structInfo();
    // DICompositeType* CTy = findStructDebugInfo(*irx.M, "thread");
    
    // BasicTypeInfo info = irx.getStructFieldInfo("thread", {"id"});
    // std::cout << info.basicType << ", " << info.size_in_bytes << std::endl;
    
    // info = irx.getStructFieldInfo("thread", {"ptr"});
    // std::cout << info.basicType << ", " << info.size_in_bytes << std::endl;
    
    // info = irx.getStructFieldInfo("thread", {"base", "id"});
    // std::cout << info.basicType << ", " << info.size_in_bytes << std::endl;
    
    // BasicTypeInfo info = irx.getStructFieldInfo("thread", {"base", "join_waiters", "prev"}); // make a unittest out of this
    // std::cout << info.basicType << ", " << info.size_in_bytes << std::endl;

    // BasicTypeInfo info = irx.getStructFieldInfo("thread", {"base", "pended_on"}); // make a unittest out of this
    // std::cout << info.basicType << ", " << info.size_in_bytes << std::endl;

    
    // char m[4096];
    
    // void *Memory = irx.structAlloc("struct.k_thread");
    
    // auto alloc = irx.structAlloc("struct.thread");
    
    // alloc.vptr = m;
    // alloc.vvptr = (uintptr_t) m;
    
    // std::cout << "m in stack memory addr:" << (void*) m << std::endl;
    
    // auto k_thread = irx.structAlloc("struct.k_thread");
    // auto alloc_t2 = irx.structAlloc("struct.thread");
    // assert(alloc.vptr == irx.vvptr2vptr(alloc.vvptr));
//     void *Memory = alloc.vptr;
//     
//     // set the arbitary pointer to an other thread, for testing in the target program
//     // uint32_t addr = 0xcafecafe;
//     // irx.irx_struct_set("thread", Memory , {"ptr"}, &addr, sizeof(uint32_t));
//     irx.irx_struct_set("thread", Memory , {"ptr"}, &alloc_t2.vvptr, sizeof(uint32_t));
// 
//     
//     //irx.irx_struct_init("thread", Memory, fields, numFields);
// 
//     // unsigned value = 1111;
//     // irx.irx_struct_set("thread", Memory , {"base", "id"}, &id, 4); // GEP: 0 0 2
//     irx.irx_struct_set("thread", Memory , {"id"}, &id, 4); // GEP: 0 1
//     irx.irx_struct_set("thread", alloc_t2.vptr , {"id"}, &id, 4); // GEP: 0 1
//     
//     irx.irx_struct_set("thread", Memory , {"base", "id"}, &idUL, sizeof(unsigned long long int));
    
    // uintptr_t ptrVal = 0x00;
    // irx.irx_struct_set("thread", Memory , {"base", "join_waiters", "prev"}, &ptrVal, 8); // GEP: 0 0 4 1 0

    // irx.irx_struct_set("thread", Memory , {"base", "join_waiters", "head"}, &ptrVal, 8); // GEP: 0 0 4 0 0

    // int array[8] = {1,2,3,4,5,6,7,8};
    // irx.irx_struct_set("thread", Memory , {"base", "n"}, &array, sizeof(array)); // GEP: 0 0 2

    // irx.irx_struct_set("thread", Memory , {"base", "n", "6"}, &array[6], sizeof(int)); // GEP: 0 0 2 6 = 24
    // irx.irx_struct_set("thread", Memory , {"base", "n", "7"}, &array[7], sizeof(int)); // GEP: 0 0 2 7 = 28

    // irx.irx_struct_set("thread", Memory , {"base", "cpus2", "1", "n"}, &ptrVal, sizeof(int));

    // // todo how to do this ?
    // void *n = irx.irx_get_global_addr("n");
    // irx.irx_struct_set("int", n , {"0"}, &ptrVal, sizeof(int));

    // irx.irx_struct_set("k_thread", Memory , {"name"}, &ptrVal, 8); // GEP: 0 0 1 0 0
    // return 0;

    // END init struct
    
    // GenericValue Result = PTOGV(Memory);

    // Value Result2 = {};
    // StructType *Ty = StructType::getTypeByName(irx.Context, "struct.thread");
    // irx.I->irx_print_struct(Ty, (Value*) &Result2);
    
    // return 0;
    
    // struct FieldValue globals_primitives = { "_current", &Memory, sizeof(Memory) };
    
    // irx.irx_set_global(globals_primitives);
 
    
    // std::vector<GenericValue> Args(0);
    
    // std::vector<GenericValue> Args(2);
    // Args[0].IntVal = APInt(32, 1000);
    // Args[1].IntVal = APInt(32, 2000);
    
    // StoreValueToMemory(Result, (GenericValue *)GVTOP(APInt(32, 42)), SI.getOperand(0)->getType());
    
    // GenericValue GVAggrThread = createStructAsGenericValueUsingAggregateVal();
    
//     std::vector<GenericValue> Args(2);
//     // Args[0] = PTOGV(&GVAggrThread);
//     Args[0] = Result;
//     // Args[0] = PTOGV((void*)alloc.vvptr);
//     // Args[1].IntVal = APInt(32, 42, false);
//     Args[1].IntVal = APInt(64, 42, false);
//     
//     // struct GenericValue argGV;
//     // struct GenericValue pstrGV;
//     // argGV.IntVal = APInt(32, 1);
//     // // pstrGV.PointerTy = constStr->BlockAddressVal;
//     // Args[0].AggregateVal.push_back(argGV);
// 
//     
//     irx.runFunction(FunctionName, Args);

    // irx.allocator->free(alloc.vvptr);
    
    printf("OK\n");

    // irx.setPrintLLVMIR(true);
    for(int i = 0; i < 10; ++i) {
        // irx.createInterpreter();
        auto t1 = std::chrono::high_resolution_clock::now();
        // irx.getStructFieldInfo2("k_thread", {"base", "thread_state"});
        auto k_thread = irx.structAlloc("struct.k_thread");
        std::cout << k_thread.vvptr << std::endl;
        
        irx.runFunction("sys_clock_driver_init", {});
        
        irx.resetExtraAllocations();
        auto now = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(now - t1).count() << " ... " << std::endl << std::flush;
    }
        
    llvm_shutdown();

    return 0;
}
