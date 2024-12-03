#include <cstdint>
#include <memory>
#include <iostream>
#include <algorithm>
#include <iomanip>

#include <llvm/IR/Constants.h>
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
#include <llvm/IR/GetElementPtrTypeIterator.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/Debug.h>
#include "llvm/BinaryFormat/Dwarf.h"
#include <llvm/Linker/Linker.h>
// #include "llvm/Support/raw_ostream.h"
#include <llvm/Support/FileSystem.h>

#include "irinterpreter2-mmap.h"
#include "irinterpreter2-struct.h"


using namespace llvm;

#define DEBUG_TYPE "IRXInterpreter"

GenericValue irx_get_global_var(MyInterpreter *EE, const char *name);

void print_bytes(char *ptr, size_t n) {
    while(n--) {
        std::cout << "[" << std::hex << (void*) ptr << "]: "
            << (unsigned int) (*(ptr) & 0xFF) << std::dec << std::endl;
        ++ptr;
    }
    // std::cout << std::endl;
}

void print_stack(std::string prefix, std::vector<ExecutionContext> &ECStack) {
    for (auto &SF : ECStack) {
        outs() << prefix << ":" << magenta << "Stack: " << SF.CurFunction->getName().str() << reset << "\n";
    }
}

std::string getCurrentFunctionForPrint(ExecutionContext &SF) {
    std::stringstream ss;
    ss << std::setw(25) << SF.CurFunction->getName().str();
    return ss.str();
}

bool contains(std::vector<std::string> haystack, std::string needle) {
    if(std::find(haystack.begin(), haystack.end(), needle) != haystack.end()) {
        return true;
    }
    return false;
}

MyInterpreter::MyInterpreter(std::unique_ptr<llvm::Module> M)
  : Interpreter(std::move(M))
{}

MyInterpreter::~MyInterpreter() {
    // undo updateGlobalMappings
    for (auto &Global : Modules.front()->globals()) {
        if (!Global.isDeclaration()) {
            uintptr_t vvptr = (uintptr_t) getPointerToGlobalIfAvailable(&Global);
            if(vvptr != 0) {
                void *vptr = allocator->vvptr2vptr(vvptr);
                if(vptr != nullptr) {
                    
                    updateGlobalMapping(&Global, vptr);
                    
                    // allocator->remove(vvptr);
                } else {
                    std::cerr << "vvptr2vptr failed" << std::endl;
                }
            }
        }
    }
    
    for (auto &F : Modules.front()->functions()) {
        if (!F.isDeclaration()) {
            void *func = getPointerToFunction(&F);
            if (func) {
                // allocator->remove();
            }
        }
    }
    
    // for (GlobalObject &GO : Modules.front()->global_objects()) {
    //     // EEState.RemoveMapping(getMangledName(&GO));
    //     GO.dump();
    // }
    
    // clearGlobalMappingsFromModule(Modules.front().get());
    
    // EEState.getGlobalAddressMap();
}

GenericValue MyInterpreter::getConstantValue(const Constant *C) {
    // LLVM::Interpreter does not expect an alias that follows a bitcast
    // this will fix LLVM-IR that executes/resolves in this order:
    // getelementptr [1] -> alias [2] -> bitcast [3]
    // At [3] the LLVM::Interpreter will only accept a pointer type
    
    auto &DL = getDataLayout();
    if (const ConstantExpr *CE = dyn_cast<ConstantExpr>(C)) {
        Constant *Op0 = CE->getOperand(0);
        switch (CE->getOpcode()) {
        // LLVM BUG FIX: If getelementptr contains an alias that follows a bitcast, LLVM::Interpreter will fail
        // therefore we "hook" in to the GEP instruction so we can execute our modified getConstantValue method
        // This code part for GEP is a copy from LLVM::ExecutionEngine::getConstantValue and is unmodified
        case Instruction::GetElementPtr: {
            // Compute the index
            GenericValue Result = getConstantValue(Op0);
            APInt Offset(DL.getPointerSizeInBits(), 0);
            cast<GEPOperator>(CE)->accumulateConstantOffset(DL, Offset);

            char* tmp = (char*) Result.PointerVal;
            Result = PTOGV(tmp + Offset.getSExtValue());
            return Result;
            break;
        }
        default:
            break;
        }
    } else {
        switch (C->getType()->getTypeID()) {
        case Type::PointerTyID:
            // LLVM::Interpreter does not expect a pointer type to have a constant expr. (e.g. bitcast)
            // executing this code will fail,
            // but it is valid LLVM-IR code (llvm ir doc: "can be an expression" & clang compiles it)
            // => The LLVM Interpreter simply does not implement this kind of code.
            
            // resolving the alias is sufficient and execution of the original code will work as expected.
            
            while (auto *A = dyn_cast<GlobalAlias>(C)) {
                C = A->getAliasee();
            }
            
            if(const Function *F = dyn_cast<Function>(C)) {
                // make sure constant expressions of functions resolve to the vvptr
                return PTOGV((void*)vptr2vvptr(getPointerToFunctionOrStub(const_cast<Function*>(F))));
            }
            
            break;
        default:
            break;
        }
    }
    
    return Interpreter::getConstantValue(C);
}

Constant* resolveAliasesOperand(Constant *V) {
    while (auto *A = dyn_cast<GlobalAlias>(V)) {
        V = A->getAliasee();
    }
    return V;
}

// fix the LLVM Interpreter alias bug
void fixAliasesOperands(Constant *V, std::map<Constant *, Constant *> &Values) {
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(V)) {
        for(size_t i = 0; i < CE->getNumOperands(); ++i) {
            Constant *C = CE->getOperand(i);
            Constant *Op = resolveAliasesOperand(C);
            
            // if(Op != C) {
            //     errs() << yellow << "fix alias:";
            //     C->dump();
            //     errs() << "->";
            //     Op->dump();
            //     errs() << reset << "\n";
            // }
            
            Values[Op] = C;
            CE->setOperand(i, Op);
            
            fixAliasesOperands(Op, Values);
        }
    }
}

void restoreAliasesOperands(Constant *V, std::map<Constant *, Constant *> &Values) {
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(V)) {
        for(size_t i = 0; i < CE->getNumOperands(); ++i) {
            Constant *C = CE->getOperand(i);
            CE->setOperand(i, Values[C]);
            restoreAliasesOperands(C, Values);
        }
    }
}


// fix the LLVM Interpreter alias bug
GenericValue MyInterpreter::getOperandValue(Value *V, ExecutionContext &SF) {
    if (ConstantExpr *CE = dyn_cast<ConstantExpr>(V)) {
        
        std::vector<Constant *> Operands(CE->getNumOperands());
        // std::map<Value *, GenericValue> Values;
        
        for(size_t i = 0; i < CE->getNumOperands(); ++i) {
            Constant *C = CE->getOperand(i);
            Operands.at(i) = C;
            
            while (auto *A = dyn_cast<GlobalAlias>(C)) {
                C = A->getAliasee();
            }
            
            CE->setOperand(i, C);
        }
        
        // now getConstantExprValue can execute without aliases
        GenericValue GV = getConstantExprValue(CE, SF);
        
        for(size_t i = 0; i < CE->getNumOperands(); ++i) {
            CE->setOperand(i, Operands.at(i));
        }
        
        return GV;
    } else if (Constant *CPV = dyn_cast<Constant>(V)) {
        return getConstantValue(CPV);
    }
    return Interpreter::getOperandValue(V, SF);
}

void MyInterpreter::visitLoadInst(LoadInst &LI) {
    ExecutionContext &SF = ECStack.back();
    GenericValue Result;
    
    llvm::Value* PointerOperand = LI.getPointerOperand();
    
    GenericValue SRC = getOperandValue(PointerOperand, SF);
    GenericValue *vvptr = (GenericValue*)GVTOP(SRC);
    // GenericValue *Ptr = (GenericValue *) allocator->vvptr2vptr((uintptr_t)GVTOP(SRC));
    
    GenericValue *Ptr = (GenericValue *) vvptr2vptr((uintptr_t)vvptr);
    // auto Ty = LI.getOperand(0)->getType();
    auto Ty = LI.getType();
    
    bool debug_load = false;
    
    if(debug_load) {
        std::cout << "LOAD: [" << (void*)vvptr << " => " << (void*)Ptr << "]" << "\n";
    }
    
    LoadValueFromMemory(Result, Ptr, Ty);
    
    if(debug_load) {
        std::cout << "LOAD: [" << (void*)vvptr << " => " << (void*)Ptr << "] <- (PTR) "
            << (void*) Result.PointerVal << "\n";
        std::cout << "LOAD: [" << (void*)vvptr << " => " << (void*)Ptr << "] <- (INT) "
            << (void*) Result.IntVal.getSExtValue() << "\n";
    }
    
    SF.Values[&LI] = Result;
    
}

void MyInterpreter::visitStoreInst(StoreInst &SI) {
    ExecutionContext &SF = ECStack.back();
    // llvm::Value* PointerOperand = SI.getPointerOperand();
    
    // SF.Values.at(PointerOperand)
    // auto it = SF.Values.find(PointerOperand);
    // if (it == SF.Values.end()) {
    //     llvm::outs() << yellow << "WARNING: " << PointerOperand << " Value does not existst on the stack" << "\n" << reset;
    // }
    
    GenericValue Val = getOperandValue(SI.getOperand(0), SF);
    GenericValue SRC = getOperandValue(SI.getPointerOperand(), SF);
    
    GenericValue *Ptr = (GenericValue *) GVTOP(SRC);
    // GenericValue *Ptr = (GenericValue *) allocator->vvptr2vptr((uintptr_t)GVTOP(SRC));
    
    Ptr = (GenericValue *) vvptr2vptr((uintptr_t)Ptr);
    
    Type *Ty = SI.getOperand(0)->getType();
    
    if (Ty->isPointerTy()) {
        llvm::Type *elementType = Ty->getPointerElementType();
        if (elementType->isFunctionTy()) {
            GenericValue *GV = (GenericValue *) vptr2vvptr(Val.PointerVal);
            
            if(GV) {
                Val.PointerVal = GV;
                Value *F = SI.getOperand(0);
                DBGS() << green << "visitStoreInst: Function pointer detected vptr2vvptr: " << F->getName().str() << " " << Val.PointerVal << reset << "\n";
            } else {
                LLVM_DEBUG(DBGS() << "Store of pointer unchanged" << "\n");
            }
        }
    }
    
    StoreValueToMemory(Val, Ptr,
                        SI.getOperand(0)->getType());
    
    // llvm::outs() << "STORE: @ " << *PointerOperand << "\n";
    // llvm::outs() << "STORE: " << SI << "\n";
    // llvm::outs() << "STORE: vv[" << SRC.PointerVal << "] <- (PTR) " << Val.PointerVal << "\n";
    // llvm::outs() << "STORE: vv[" << SRC.PointerVal << "] <- (INT) " << Val.IntVal.getSExtValue() << "\n";
    // llvm::outs() << "STORE: v[" << (uint8_t*)Ptr << "] <- (INT) " << Val.IntVal.getSExtValue() << "\n";
}

Function* MyInterpreter::findFunctionByVPtr(void* vptr) {
    for (auto &F : Modules.front()->functions()) {
        if (!F.isDeclaration()) {
            void *func = getPointerToFunction(&F);
            if(func == vptr) {
                return &F;
            }
        }
    }
    return nullptr;
}

Function* MyInterpreter::findFunctionByVVPtr(uintptr_t vvptr) {
    return findFunctionByVPtr(vvptr2vptr(vvptr));
}

std::string IRModuleInterpreter::findFunctionByVVPtr(uintptr_t vvptr) {
    std::string name = "";
    Function *F = I->findFunctionByVPtr(vvptr2vptr(vvptr));
    if(F) {
        name = F->getName().str();
    }
    return name;
}


std::string IRModuleInterpreter::getGlobalValueAtAddress(uintptr_t vvptr) {
    const GlobalValue *GV = I->getGlobalValueAtAddress(vvptr2vptr(vvptr));
    
    if(GV) {
        return GV->getName().str();
    }
    
    return "";
}


void MyInterpreter::visitCallBase(CallBase &call) {
    ExecutionContext &SF = ECStack.back();
    auto function = getCurrentFunctionForPrint(SF);
    
    SF.Caller = &call;

    std::vector<GenericValue> ArgVals;
    const unsigned NumArgs = SF.Caller->arg_size();
    ArgVals.reserve(NumArgs);
    for (Value *V : SF.Caller->args()) {
        GenericValue v = getOperandValue(V, SF);
        
        ArgVals.push_back(v);

        // std::cout << v.IntVal.getSExtValue() << std::endl;
        
        // irx_get_global_var(this, "_current");
    }

    // for (auto arg = call.args().begin();arg != call.args().end(); ++arg) {
    //     std::cout << arg->get()->getName().str() << std::endl;
    //     // std::cout << arg->get()->getValueName()->getValue() << std::endl;
    //     std::cout << arg->getOperandNo() << std::endl;
    // }
    // TODO call if needed (also from array provided by user)
    
    
    
    // Type *Ty = call.getCalledOperand()->getType();
    // call.getCalledOperand()->dump();
    
    auto *F = call.getCalledFunction();
    // !F->isDeclaration() &&
    // && !F->getName().startswith("memcpy")
    if (F == nullptr) {
        GenericValue V = getOperandValue(call.getCalledOperand(), SF);
        V.PointerVal = (GenericValue *) vvptr2vptr((uintptr_t)V.PointerVal);
        if(V.PointerVal) {
            SF.Values[call.getCalledOperand()] = V;
            
            if(Function *F = findFunctionByVPtr(V.PointerVal)) {
                DBGS() << green << "visitCallBase: Function pointer detected vptr2vvptr: " << F->getName().str() << " " << V.PointerVal << reset << "\n";
            
                visit(call);
            } else {
                print_stack("visitCallBase findFunctionByVPtr", ECStack);
#ifdef LLVM_ENABLE_DUMP
                call.dump();
#endif // LLVM_ENABLE_DUMP
                errs() << red << "warning: Function not found skipping!" << reset << "\n";
            }
        } else {
            print_stack("visitCallBase vvptr2vptr", ECStack);
#ifdef LLVM_ENABLE_DUMP
                call.dump();
#endif // LLVM_ENABLE_DUMP
            errs() << red << "warning: nullptr function: skipping" << reset << "\n";
            Type *RTy = call.getType();
            if (!RTy->isVoidTy()) {
                errs() << red << "warning: and is not void" << reset << "\n";
                if (RTy->isIntegerTy()) {
                    errs() << red << "warning: setting zero" << reset << "\n";
                    GenericValue GV = GenericValue();
                    GV.IntVal = APInt(RTy->getIntegerBitWidth(), 0);
                    SF.Values[&call] = GV;
                }
            }
        }
    } else if (!F->getName().startswith("llvm.")) {
        // TODO print func signature
        // dbgs() << F-> << "\n";
        DBGS() << green;
        for(auto v : ArgVals){
            DBGS() << v.PointerVal << ", ";
        }
        DBGS() << reset << "\n";
        
        // ExecutionContext &SF = ECStack.back();
        //
        // SF.Caller = &I;
        // std::vector<GenericValue> ArgVals;
        // const unsigned NumArgs = SF.Caller->arg_size();
        // ArgVals.reserve(NumArgs);
        // for (Value *V : SF.Caller->args())
        //   ArgVals.push_back(getOperandValue(V, SF));
        //
        // // To handle indirect calls, we must get the pointer value from the argument
        // // and treat it as a function pointer.
        // GenericValue SRC = getOperandValue(SF.Caller->getCalledOperand(), SF);
        // callFunction((Function*)GVTOP(SRC), ArgVals);

        // Interpreter::visitCallBase(call);
        // std::cout << "calling function: " << F->getName().str() << std::endl;
        // visitCallBase(call);
        visit(call);
        
        if(flowControl->printLLVMIR) {
            print_stack(function, ECStack);
        }
    } else if(F && F->getName().startswith("llvm.")) {
        if(F->getName().startswith("llvm.dbg.declare")) {}
        else if (F->getName().startswith("llvm.dbg.value")) {}
        else if (F->getName().startswith("llvm.experimental.noalias.scope.decl")) {}
        else {
            // errs() << yellow << "warning: ignoring: " << F->getName() << reset << "\n";
            visit(call);
        }
    } else {
        errs() << yellow << "warning: ignoring: " << F->getName() << reset << "\n";
    }
}

void MyInterpreter::LoadValueFromMemory(GenericValue &Result,
                                        GenericValue *Ptr,
                                        Type *Ty) {
    if((uintptr_t)Ptr < 0x100) {
        std::cout << red << "Possible nullptr access detected " << Ptr << reset << std::endl;
        print_stack("LoadValueFromMemory", ECStack);
    }
    
    const unsigned StoreBytes = getDataLayout().getTypeStoreSize(Ty);
    
    if (Ty->getTypeID() == Type::PointerTyID && StoreBytes != sizeof(PointerTy)) {
        // print_bytes((char*)Ptr, StoreBytes);
        DBGS() << cyan << "MyInterpreter LoadValueFromMemory: handeling loading Pointer with custom memcpy "
            << StoreBytes << reset << "\n";
        memset(&(Result.PointerVal), 0, sizeof(PointerTy));
        // size_t diff = sizeof(PointerTy) - StoreBytes;
        // memcpy((void*)((uintptr_t)(&Result.PointerVal) + diff), Ptr, StoreBytes);
        memcpy((void*)((uintptr_t)(&Result.PointerVal)), Ptr, StoreBytes);
        // print_bytes((char*)&Result.PointerVal, StoreBytes);
    } else {
        ExecutionEngine::LoadValueFromMemory(Result, Ptr, Ty);
    }
}

void MyInterpreter::StoreValueToMemory(const GenericValue &Val, GenericValue *Ptr, Type *Ty) {
    if((uintptr_t)Ptr < 0x100) {
        std::cout << red << "Possible nullptr access detected" << reset << "\n";
        print_stack("StoreValueToMemory", ECStack);
    }
    
    const unsigned StoreBytes = getDataLayout().getTypeStoreSize(Ty);
    
    if (Ty->getTypeID() == Type::PointerTyID && StoreBytes != sizeof(PointerTy)) {
        // ExecutionEngine::StoreValueToMemory(Val, Ptr, Type::IntegerTyID); will not work, as PointerVal is not stored in APInt
        // print_bytes((char*)Ptr, StoreBytes);
        DBGS() << cyan << "MyInterpreter StoreValueToMemory: handeling loading Pointer with custom memcpy "
            << StoreBytes << reset << "\n";
        // print_bytes((char*)&Val.PointerVal, StoreBytes);
        DBGS() << "MyInterpreter StoreValueToMemory PTR " << Ptr << " " << Val.PointerVal << "\n";
        
        memcpy((void*)(Ptr), &Val.PointerVal, StoreBytes);
        // memcpy(GVTOP(*Ptr), &Val.PointerVal, StoreBytes);
        // print_bytes((char*)Ptr, StoreBytes);
    } else {
        ExecutionEngine::StoreValueToMemory(Val, Ptr, Ty);
    }
}

// return true if run should stop
bool MyInterpreter::runInstruction(Instruction &I) {
    bool ret = false;
    ExecutionContext &SF = ECStack.back();
    auto function = getCurrentFunctionForPrint(SF);
    
    bool skip_printing = false;
    if(I.getOpcode() == Instruction::Call) {
        CallBase &call = static_cast<CallBase&>(I);
        auto *F = call.getCalledFunction();
        if(F) {
            if(F->getName().startswith("llvm.dbg") || F->getName().startswith("llvm.experimental")) {
                skip_printing = true;
            }
        }
    }
    
    if(!flowControl->printLLVMIR) {
        skip_printing = true;
    }
    
    // "%20s:\t"
    if(!skip_printing) {
        auto color = reset;
        if(I.getOpcode() == Instruction::Ret) color = pink;
        if(I.getOpcode() == Instruction::Call) color = pink;
        if(I.getOpcode() == Instruction::Load) color = yellow;
        if(I.getOpcode() == Instruction::Store) color = blue;
        if(I.getOpcode() == Instruction::GetElementPtr) color = white;
        outs() << function << ":\t" << color;
        I.print(outs());
        outs() << reset << '\n';
    }
    
    // print source code info
    if(!skip_printing) {
        if (auto dbg = I.getDebugLoc()) {
            DIScope *scope = dbg->getScope();
            StringRef file = dbg->getFilename();
            unsigned line = dbg->getLine();
            
            while (scope) {
                if (auto *subprogram = llvm::dyn_cast<llvm::DISubprogram>(scope)) {
                    errs() << file << ":" << subprogram->getName() << ":" << line << "\n";
                }
                scope = scope->getScope();
            }
        }
    }
    
    
    std::map<Constant *, Constant *> Values;
    
    if(I.getOpcode() != Instruction::Call) {
        for(size_t i = 0; i < I.getNumOperands(); ++i) {
            if (Constant *C = dyn_cast<Constant>(I.getOperand(i))) {
                fixAliasesOperands(C, Values);
            }
        }
    }
    
    if(I.getOpcode() == Instruction::Ret) {
        visit(I);
        if(flowControl->printLLVMIR) {
            print_stack(function, ECStack);
        }
    }
    else if(I.getOpcode() == Instruction::GetElementPtr) {
        // GetElementPtrInst &GEP = static_cast<GetElementPtrInst&>(I);
        // Value *V = GEP.getPointerOperand();
        // GenericValue *GV = &SF.Values[V];
        
        // If our mapping has this value in our map, replace it
        // could also be placed either in 
        // - load
        // - store
        // but at this point its to late and the offset is alread calculated on top, so we need to do an (ranged lower/upper) interception
        // better way would be probably to MAP-translate the loaded value if it is a pointer, will this work?
        
        visit(I);
    } else if(I.getOpcode() == Instruction::Load) {
        LoadInst &LI = static_cast<LoadInst&>(I);
        visitLoadInst(LI);
    }
    else if(I.getOpcode() == Instruction::Store) {
        StoreInst &SI = static_cast<StoreInst&>(I);
        visitStoreInst(SI);
    } else if(I.getOpcode() == Instruction::Call) {
        CallBase &call = static_cast<CallBase&>(I);
        auto *F = getFunctionWithPtr(call);
        if(F && flowControl->shouldStop(F)) {
            ret = true;
            goto runExit;
        }
        
        if(F && flowControl->shouldSkip(F)) {
            goto runExit;
        }
        
        visitCallBase(call);
    } else {
        visit(I);
    }
    
runExit:
        if(I.getOpcode() != Instruction::Call) {
            for(size_t i = 0; i < I.getNumOperands(); ++i) {
                if (Constant *C = dyn_cast<Constant>(I.getOperand(i))) {
                    restoreAliasesOperands(C, Values);
                }
            }
        }
    return ret;
}

void MyInterpreter::run() {
    while (!ECStack.empty()) {
        ExecutionContext &SF = ECStack.back();  // Current stack frame
        Instruction &I = *SF.CurInst++;         // Increment before execute
        
        runningInstruction = &I;
        
        if(runInstruction(I)) {
            return;
        }
    }
    
    DBGS() << "EXIT VAL: " << ExitValue.IntVal.getSExtValue() << "\n";
}

GenericValue MyInterpreter::getExitValue() {
    return ExitValue;
}

GenericValue IRModuleInterpreter::getExitValue() {
    return I->getExitValue();
}

std::vector<std::string> IRModuleInterpreter::getCallStack() {
    return I->getCallStack();
}

std::vector<std::string> MyInterpreter::getCallStack() {
    std::vector<std::string> stack;
    for (auto &SF : ECStack) {
        stack.push_back(SF.CurFunction->getName().str());
    }
    return stack;
}

std::vector<GenericValue> MyInterpreter::getCurrentCallArgs() {
    std::vector<GenericValue> ArgVals;
    
    if(!ECStack.empty()) {
        ExecutionContext &SF = ECStack.back();
        
        if(runningInstruction->getOpcode() == Instruction::Call) {
            const unsigned NumArgs = SF.Caller->arg_size();
            ArgVals.reserve(NumArgs);
            for (Value *V : SF.Caller->args()) {
                GenericValue v = getOperandValue(V, SF);
                ArgVals.push_back(v);
            }
        }
    }
    
    return ArgVals;
}

std::vector<GenericValue> IRModuleInterpreter::getCurrentCallArgs() {
    return I->getCurrentCallArgs();
}

Function* MyInterpreter::getFunctionWithPtr(CallBase &call) {
    Function *F = call.getCalledFunction();
    if(F) {
        return F;
    } else {
        ExecutionContext &SF = ECStack.back();
        Value *calledOperand = call.getCalledOperand();
        GenericValue v = getOperandValue(calledOperand, SF);
        F = findFunctionByVVPtr((uintptr_t)v.PointerVal);
    }
    return F;
}

std::string MyInterpreter::getCurrentCallFunctionName() {
    std::string name = "";
    if(runningInstruction->getOpcode() == Instruction::Call) {
        CallBase &call = static_cast<CallBase&>(*runningInstruction);
        name = getFunctionWithPtr(call)->getName().str();
    }
    
    return name;
}

std::string IRModuleInterpreter::getCurrentCallFunctionName() {
    return I->getCurrentCallFunctionName();
}

bool IRModuleInterpreter::isRunnable() {
    return I->isRunnable();
}

bool MyInterpreter::isRunnable() {
    return !ECStack.empty();
}

// TODO alloca will still be in 64-bit space
char *MyInterpreter::getMemoryForGV(const GlobalVariable *GV) {
    char *GA = ExecutionEngine::getMemoryForGV(GV);
    
    // called in Interpreter::ctor, so virtual function will not be called...
    
//     std::cout << bold << "getMemoryForGV: " << GV->getName().str() << reset << std::endl;
//     
//     Type *Ty = GV->getValueType();
//     size_t size = (size_t)getDataLayout().getTypeAllocSize(Ty);
//     struct irx_alloc alloc = allocator->add(GA, size);
//     return (char*) alloc.vvptr;
    
    return GA;
}


void IRModuleInterpreter::resetExtraAllocations() {
    I->resetExtraAllocations();
}

void MyInterpreter::resetExtraAllocations() {
    for(uintptr_t vvptr : allocations) {
        std::cout << (void*) vvptr << std::endl;
        allocator->free(vvptr);
    }
    
    allocations.clear();
    
    allocator->reset();
    allocator->freeze();
    
    ECStack.clear();
    ExitValue = GenericValue();
    
    auto empty = std::map<std::string, bool>();
    InitializeGlobals(empty);
}

int IRModuleInterpreter::getPointerSize() {
    const llvm::DataLayout *DL;
    if(M) {
        DL = &M->getDataLayout();
    } else {
        return 64;
        // std::unique_ptr<llvm::Module> Module = std::make_unique<llvm::Module>("native", Context);
        // Module->setTargetTriple(llvm::sys::getProcessTriple());
        // Module->setTargetTriple("x86_64-unknown-linux-gnu");
        // DL = &Module->getDataLayout();
    }
    return DL->getPointerSizeInBits();
}

IRModuleInterpreter::IRModuleInterpreter() {
    M = std::make_unique<Module>("empty", Context);
    
    DBGS() << "IRModuleInterpreter::IRModuleInterpreter()" << "\n";
    
    // llvm::InitializeNativeTarget();
    // llvm::InitializeNativeTargetAsmPrinter();
    // llvm::InitializeNativeTargetAsmParser();
    
    setCurrentDebugType(DEBUG_TYPE);
}

IRModuleInterpreter::~IRModuleInterpreter() {
    if(I) {
        delete I;
    }
}

std::vector<std::string> getExternalSymbolsInModule(Module *M) {
    std::vector<std::string> declarations;
    for (auto &Global : M->globals()) {
        if (Global.isDeclaration()) {
            declarations.push_back(Global.getName().str());
        }
    }
    return declarations;
}

void MyInterpreter::replaceModule(std::unique_ptr<llvm::Module> M) {
    removeModule(&(*Modules.back()));
    addModule(std::move(M));
}

// application LLVM-IR file should be first, since struct will get named with a prefix and collectLinkerScriptSections will fail
bool IRModuleInterpreter::parseIRFiles(std::vector<std::string> InputFilenames, std::string name) {
    // Module *composite = new Module(name, Context);
    Module *composite = M.get();
    // M->print(llvm::outs(), nullptr);
    // Linker::linkModules(*composite, std::move(M));

    for(auto InputFilename: InputFilenames) {
        std::cout << "Loading IR-File " << InputFilename << "\n";
        auto m = llvm::parseIRFile(InputFilename, Err, Context);
        if (!m) {
             std::cerr << "Module could not be loaded " << InputFilename << std::endl;
            return false;
        }

        if(verifyModule(*m, &errs())) {
            std::cerr << "Module verification failed" << std::endl;
            m->print(outs(), nullptr);
            return false;
        }

        Linker::linkModules(*M, std::move(m));
    }

    if(verifyModule(*composite, &errs())) {
        std::cerr << "Module verification failed" << std::endl;
        composite->print(outs(), nullptr);
        return false;
    }
    
    // fix unnamed GlobalVariables
    for (auto &Global : composite->globals()) {
        if (!Global.hasName()) {
            auto t = Twine("unnamed");
            Global.setName(t.getSingleStringRef());
        }
    }
    
    // make sure to run the sections linker only once
    if(composite->getNamedGlobal("initlevel") == nullptr) {
        composite = collectLinkerScriptSections(composite);
    }
    
    for (auto &G : composite->globals()) {
        if (G.hasExternalLinkage() && G.isDeclaration()) {
            G.setInitializer(Constant::getNullValue(G.getType()->getPointerElementType()));
            G.setLinkage(GlobalValue::ExternalLinkage);
        }
    }
    
    // M = std::unique_ptr<Module>(composite);
    
    DIF.processModule(*M);
    
    // Linker::linkModules(*M, std::unique_ptr<Module>(MG), Linker::Flags::OverrideFromSrc);
    verifyModule(*M, &errs());
    
    // TODO set stdout stderr
    
    // dump module
    std::error_code EC;
    raw_fd_ostream fd("/tmp/interpreter-composite.ll", EC);
    M->print(fd, nullptr, true, false);
    fd.close();
    // M->print(llvm::outs(), nullptr);
    
    
    auto externalGlobals = getExternalSymbolsInModule(M.get());
    
    if(externalGlobals.size() > 0) {
        for(auto g : externalGlobals) {
            std::cerr << "Missing external global: " << g << std::endl;
        }
        // return false;
    }
    
    for (auto &G : M->globals()) {
        if (G.hasExternalLinkage() && G.isDeclaration()) {
            G.setInitializer(Constant::getNullValue(G.getType()->getPointerElementType()));
            G.setLinkage(GlobalValue::ExternalLinkage);
        }
    }

    return true;
}

std::vector<std::string> IRModuleInterpreter::getDeclaredFunction() {
    std::vector<std::string> functions;
    
    for(auto &F : M->functions()) {
        if(F.isDeclaration()){
            functions.push_back(F.getName().str());
        }
    }
    
    return functions;
}

bool IRModuleInterpreter::isFunctionDeclared(std::string name) {
    auto v = getDeclaredFunction();
    return std::find(v.begin(), v.end(), name) != v.end();
}

bool IRModuleInterpreter::createInterpreter() {
    // HACK: to support 32-bit pointers on 64-bit hosts
    // InitializeMemory would write to memory with the native pointer size.
    // to prevent this we will fake the `isThreadLocal` value to prevent the LLVM::Interpreter
    // to initialize the GlobalVariables and do so ourselfs later using the updates pointer values and
    // using methods that will write pointers with the correct size.
    
    // This will skip the InitializeMemory method call and prevent
    // GlobalVariables to get initialized in emitGlobal called from the Interpreter's ctor.
    // Save there current value in `isThreadLocal` to restore later.
    std::map<std::string, bool> isThreadLocal = fakeSetIsThreadLocalGlobals(M.get());
    
    I = new MyInterpreter(CloneModule(*M));
    
    // auto allocator = new LLVMGlobalVariableAllocator();
    // auto allocator = std::make_unique<MMapAllocator>();
    auto allocator = std::make_unique<MapAllocator>();
    
    I->allocator = std::move(allocator);
    
    I->flowControl = &flowControl;
    
    I->updateGlobalMappings();
    
    I->InitializeGlobals(isThreadLocal);
    
    I->allocator->freeze();
    
    for (auto &GV : M->globals()) {
        GV.setThreadLocal(isThreadLocal[GV.getName().str()]);
    }
    
    return true;
}

std::map<std::string, bool> IRModuleInterpreter::fakeSetIsThreadLocalGlobals(Module *M) {
    std::map<std::string, bool> isThreadLocal;
    
    for(GlobalVariable &GV : M->globals()) {
        if(GV.hasInitializer()) { //  && !GV.getName().startswith("llvm.")
            isThreadLocal[GV.getName().str()] = GV.isThreadLocal();
            GV.setThreadLocal(true);
        }
    }
    
    return isThreadLocal;
}

void MyInterpreter::InitializeGlobals(std::map<std::string,  bool> &isThreadLocal) {
    for (auto &GV : Modules.front()->globals()) {
        if(GV.hasInitializer()) { //  && !GV.getName().startswith("llvm.")
            uintptr_t VVGA = (uintptr_t) getPointerToGlobalIfAvailable(&GV);
            if(allocator->has(VVGA)) {
                void *GA = allocator->vvptr2vptr(VVGA);
                if(GA) {
                    if(isThreadLocal.find(GV.getName().str()) != isThreadLocal.end()) {
                        GV.setThreadLocal(isThreadLocal[GV.getName().str()]);
                    } else if (!isThreadLocal.empty()) {
                        std::cerr << red << "InitializeGlobals: isThreadLocal has no mapping to GV: " << GV.getName().str() << reset <<std::endl;
                    }
                    
                    if (!GV.isThreadLocal()) {
                        // InitializeMemory uses MyInterpreter::StoreValueToMemory (handels pointer size correctly),
                        // and uses vvptr for GlobalVariables and therefore initialize values correctly
#ifdef LLVM_ENABLE_DUMP
                        LLVM_DEBUG(GV.dump());
                        LLVM_DEBUG(GV.getInitializer()->dump());
#endif // LLVM_ENABLE_DUMP
                        InitializeMemory(GV.getInitializer(), GA);
                    }
                } else {
                    std::cout << blue << "no vvptr for GV: " << GV.getName().str() << reset <<std::endl;
                }
            } else {
                std::cout << blue << "no vvmap for GV: " << GV.getName().str() << reset <<std::endl;
            }
        }
    }
    
    
    // resetLastAllocAddr = static_cast<MapAllocator*>(allocator.get())->vma.lastAllocAddr; // TODO: fix me
}

void MyInterpreter::updateGlobalMappings() {
    // GlobalVariables already have memory allocated to them,
    // thus we only need to add the mapping to the allocator
    
    for (auto &Global : Modules.front()->globals()) {
        if (!Global.isDeclaration()) {
            Type *Ty = Global.getValueType();
            size_t size = (size_t)getDataLayout().getTypeAllocSize(Ty);
            void *Ptr = getPointerToGlobalIfAvailable(&Global);
            if(Ptr) {
                struct irx_alloc alloc = allocator->add(Ptr, size);
                LLVM_DEBUG(DBGS() << "updateGlobalMappings GVName: " << Global.getName().str() << "\t" << Ptr << " " << (void*) alloc.vvptr << "\n");
                errs() << "updateGlobalMappings GVName: " << Global.getName().str() << "\t" << Ptr << " " << (void*) alloc.vvptr << "\n";
                
                updateGlobalMapping(&Global, (void*) alloc.vvptr);

            } else {
                std::cout << "GVName: " << Global.getName().str() << "\t" << Ptr << " not found" << std::endl;
            }
        }
    }
    
    for (auto &F : Modules.front()->functions()) {
        if (!F.isDeclaration()) {
            void *func = getPointerToFunction(&F);
            if (func) {
                struct irx_alloc alloc = allocator->add(func, 1);
                LLVM_DEBUG(DBGS() << "Function map: " << F.getName().str() << " " << func << " => " << alloc.vvptr << "\n");
            }
        }
    }
    
}

// This is a copy of ExecutionEngine::InitializeMemory
void MyInterpreter::InitializeMemory(const Constant *Init, void *Addr) {
  LLVM_DEBUG(dbgs() << "JIT: Initializing " << Addr << " ");
#ifdef LLVM_ENABLE_DUMP
  LLVM_DEBUG(Init->dump());
#endif // LLVM_ENABLE_DUMP
  if (isa<UndefValue>(Init))
    return;

  if (const ConstantVector *CP = dyn_cast<ConstantVector>(Init)) {
    unsigned ElementSize =
        getDataLayout().getTypeAllocSize(CP->getType()->getElementType());
    for (unsigned i = 0, e = CP->getNumOperands(); i != e; ++i)
      InitializeMemory(CP->getOperand(i), (char*)Addr+i*ElementSize);
    return;
  }

  if (isa<ConstantAggregateZero>(Init)) {
    memset(Addr, 0, (size_t)getDataLayout().getTypeAllocSize(Init->getType()));
    return;
  }

  if (const ConstantArray *CPA = dyn_cast<ConstantArray>(Init)) {
    unsigned ElementSize =
        getDataLayout().getTypeAllocSize(CPA->getType()->getElementType());
    for (unsigned i = 0, e = CPA->getNumOperands(); i != e; ++i)
      InitializeMemory(CPA->getOperand(i), (char*)Addr+i*ElementSize);
    return;
  }

  if (const ConstantStruct *CPS = dyn_cast<ConstantStruct>(Init)) {
    const StructLayout *SL =
        getDataLayout().getStructLayout(cast<StructType>(CPS->getType()));
    for (unsigned i = 0, e = CPS->getNumOperands(); i != e; ++i)
      InitializeMemory(CPS->getOperand(i), (char*)Addr+SL->getElementOffset(i));
    return;
  }

  if (const ConstantDataSequential *CDS =
               dyn_cast<ConstantDataSequential>(Init)) {
    // CDS is already laid out in host memory order.
    StringRef Data = CDS->getRawDataValues();
    memcpy(Addr, Data.data(), Data.size());
    return;
  }

  if (Init->getType()->isFirstClassType()) {
    GenericValue Val = getConstantValue(Init);
    StoreValueToMemory(Val, (GenericValue*)Addr, Init->getType());
    return;
  }

  dbgs() << "Bad Type: " << *Init->getType() << "\n";
  llvm_unreachable("Unknown constant type to initialize memory with!");
}

bool IRModuleInterpreter::resume() {
    bool ret = true;
    
    try {
        I->run();
    } catch(std::exception *e) {
        outs() << e->what() << "\n";
        ret = false;
    }

    // outs() << "Function " << FunctionName << " returned: " << GV.IntVal << "\n";
    // delete EE;
    
    return ret;
}

// void MyInterpreter::callFunction(Function *F, ArrayRef<GenericValue> ArgVals) {
//     if (F->isDeclaration()) {
//         std::cout << "MyInterpreter callFunction skip" << std::endl;
//         return;
//     }
//
//     Interpreter::callFunction(F, ArgVals);
// }

uint64_t IRModuleInterpreter::FindFunctionNamed(std::string FunctionName) {
    Function *F = I->FindFunctionNamed(FunctionName);
    if (!F) {
        errs() << "Function " << FunctionName << " not found in module.\n";
        return 0;
    }
    
    uintptr_t vvptr = vptr2vvptr((void*)F);
    
    return vvptr;
}

bool IRModuleInterpreter::runFunction(std::string FunctionName, std::vector<GenericValue> Args) {
    // MyInterpreter xI(std::move(M));
    // MyInterpreter *I = &xI;
    // ExecutionEngine *EE = &xI;
    
    Function *F = I->FindFunctionNamed(FunctionName);
    // Function *F = M->getFunction(FunctionName); // only works if module was not moved to interpreter
    if (!F) {
        errs() << "Function " << FunctionName << " not found in module.\n";
        return false;
    }
    
    for(auto arg : Args) {
        DBGS() << "arg[] PTR: " << arg.PointerVal;
        DBGS() << ", INT: " << arg.IntVal.getSExtValue() << "\n";
    }
    
    I->callFunction(F, Args);
    
    return this->resume();
}

void IRModuleInterpreter::stop(std::vector<std::string> functions) {
    flowControl.stop.assign(functions.begin(), functions.end());
}

void IRModuleInterpreter::skip(std::vector<std::string> functions) {
    flowControl.skip.assign(functions.begin(), functions.end());
}

void IRModuleInterpreter::setPrintLLVMIR(bool printit) {
    flowControl.printLLVMIR = printit;
}

bool IRXFlowControl::shouldStop(Function *F) {
    if(contains(stop, F->getName().str())) {
        DBGS() << cyan << "Flow Control: call: " << F->getName().str()
            << " requested to stop at this function" << reset << "\n";
        return true;
    }
    return false;
}

bool IRXFlowControl::shouldSkip(Function *F) {
    if(contains(skip, F->getName().str())) {
        DBGS() << cyan << "Flow Control: call: " << F->getName().str()
            << " requested to skip this function" << reset << "\n";
        return true;
    }
    return false;
}

void IRModuleInterpreter::putReturnValue(GenericValue Result) {
    return I->putReturnValue(Result);
}

// this method assumes that the call was stopped before a new stack frame was allocated
void MyInterpreter::putReturnValue(GenericValue Result) {
    // see: Interpreter::popStackAndReturnValueToCaller
    
    ExecutionContext &CallingSF = ECStack.back();
    // SetValue(CallingSF.Caller, Result, CallingSF);
    CallingSF.Values[runningInstruction] = Result;
}
