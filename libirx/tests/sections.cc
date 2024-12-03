#include <iostream>
#include "gtest/gtest.h"
#include "../irinterpreter2-struct.h"

#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Linker/Linker.h>
#include <memory>

using namespace std;
using namespace llvm;

class SectionsTest : public testing::Test {
 protected:
  SectionsTest() {
      EXPECT_TRUE(irx.parseIRFiles({"libirx/tests/test.ll"}));
  }
  
  ~SectionsTest() override {

  }
  
  IRModuleInterpreter irx;
};

TEST(EmptyTest, sections) {
  unique_ptr<Module> M;
  LLVMContext Context;
  
    M = std::make_unique<Module>("empty", Context);

    std::vector<Type*> args;

    Type *Ty = FunctionType::get(Type::getVoidTy(Context), args, false);
    Type *PtrTy = PointerType::get(Ty, 0);
    
    std::vector<const char*> sections = {
        ".native_PRE_BOOT_33_task",
        ".native_PRE_BOOT_22_task",
        ".native_PRE_BOOT_21_task",
        ".native_PRE_BOOT_13_task",
        ".native_PRE_BOOT_12_task",
        ".native_PRE_BOOT_11_task",
        ".native_FIRST_SLEEP2_task",
        ".native_FIRST_SLEEP1_task",
        ".native_ON_EXIT2_task",
        ".native_ON_EXIT1_task",
        ".nsi_ON_EXIT_POST0_task",
        ".nsi_ON_EXIT_PRE100_task",
        ".nsi_HW_INIT10_task",
        ".nsi_hw_event_0",
        ".nsi_PRE_BOOT_10_task",
        ".nsi_PRE_BOOT_11_task",
        ".nsi_PRE_BOOT_12_task",
        ".nsi_PRE_BOOT_13_task",
        ".nsi_PRE_BOOT_20_task",
        // ".z_init_PRE_KERNEL_20_0_",
        // ".z_init_PRE_KERNEL_21_0_",
        // ".z_init_PRE_KERNEL_23_0_",
        // "._k_sem.static.threadA_sem_",
        // "._k_sem.static.threadB_sem_",
        // "._k_sem.static.threadC_sem_",
        // ".__static_thread_data.static._k_thread_data_thread_a_",
        // ".__static_thread_data.static._k_thread_data_thread_b_",
    };
    
    for(auto section : sections) {
        auto GV = new GlobalVariable(*M, PtrTy, false, GlobalVariable::ExternalLinkage, Constant::getNullValue(PtrTy), "function");
        GV->setSection(section);
    }
    
    std::vector<const char*> symbols {
        "__native_PRE_BOOT_1_tasks_start",
        "__native_PRE_BOOT_2_tasks_start",
        "__native_PRE_BOOT_3_tasks_start",
        "__native_FIRST_SLEEP_tasks_start",
        "__native_ON_EXIT_tasks_start",
        "__native_tasks_end",
    };
    
    auto *packedTy = ArrayType::get(PtrTy, 0);
    
    for(auto section : symbols) {
        new GlobalVariable(*M, packedTy, false, GlobalVariable::ExternalLinkage, nullptr, section);
    }
    
    
    collectLinkerScriptSections(M.get());
    
    EXPECT_NE(M->getNamedValue("__native_PRE_BOOT_1_tasks_start"), nullptr);
    EXPECT_NE(M->getNamedValue("__native_tasks_end"), nullptr);
    // EXPECT_TRUE(M->getNamedValue("__static_thread_data_list_start"));
    // EXPECT_TRUE(M->getNamedValue("_k_sem_list_start"));
    
    M->print(outs(), nullptr);
    
    bool BrokenDebugInfo = true;
    EXPECT_FALSE(verifyModule(*M, &errs(), &BrokenDebugInfo));
    
    M = nullptr;
}

TEST_F(SectionsTest, sections) {
    Module *MG = collectLinkerScriptSections(irx.M.get());
    
    // Linker::linkModules(*irx.M, std::unique_ptr<Module>(MG), Linker::Flags::OverrideFromSrc);
    
    bool BrokenDebugInfo = true;
    EXPECT_FALSE(verifyModule(*irx.M, &errs(), &BrokenDebugInfo));
}

