#include <iostream>
#include "gtest/gtest.h"
#include "../irinterpreter2-struct.h"
#include "../irinterpreter2-mmap.h"

#include "../test-appl/simple/test.h"

using namespace std;

class IRModuleInterpreterTest : public testing::Test {
 protected:
  IRModuleInterpreterTest() {
    sbrk(0x100000000UL);
    
    EXPECT_TRUE(irx.parseIRFiles({"libirx/tests/test.ll", "libirx/tests/test-extern.ll"}));
    EXPECT_TRUE(irx.createInterpreter());
    
    auto alloc = irx.structAlloc("struct.thread");
    Memory = alloc.vptr;
    EXPECT_NE((size_t) Memory, 0);
    
    // this unittest module depends on the test.ll IR modules to be compiled for the host = assuming 64-bit host
    // if this test fails, other test may fail false negative
    // all occurences are marked with !!ATTENTION
    EXPECT_EQ(irx.getPointerSize(), 64);
  }
  
  ~IRModuleInterpreterTest() override {
    if(Memory) {
        // irx.allocator->free(irx.vptr2vvptr(Memory));
    } else {
        std::cout << "Memory PTR was null" << std::endl;
    }
  }
  
  IRModuleInterpreter irx;
  void *Memory = nullptr;
  
  uintptr_t ptrVal = 0x00;
  unsigned value = 1111;
  unsigned long long valueUL = 1111;
};

TEST(IRModuleInterpreterParseIRFilesTest, parseIRFiles) {
    IRModuleInterpreter irx;
    EXPECT_TRUE(irx.parseIRFiles({"libirx/tests/test.ll", "libirx/tests/test-extern.ll"}));
    
    auto alloc = irx.structAlloc("struct.thread");
    EXPECT_EQ(alloc.vptr, irx.vvptr2vptr(alloc.vvptr));
    EXPECT_EQ(alloc.vvptr, irx.vptr2vvptr(alloc.vptr));
    
    EXPECT_NE((size_t) alloc.vptr, 0);
    EXPECT_NE((size_t) alloc.vvptr, 0);
    
    EXPECT_EQ(alloc.size, sizeof(struct thread)); // !!ATTENTION
}

TEST_F(IRModuleInterpreterTest, set_int__base_id) {
    irx.irx_struct_set("thread", Memory , {"base", "id"}, &valueUL, sizeof(unsigned long long int)); // GEP: 0 0 5
    // irx.irx_struct_set("thread", Memory , {"base", "id"}, &value, 4); // GEP: 0 0 5
}

TEST_F(IRModuleInterpreterTest, getStructFieldInfo__base_id) {
    struct BasicTypeInfo info = irx.getStructFieldInfo("thread", {"base", "id"});
    std::vector<size_t> needle = {0, 0, 5};
    EXPECT_TRUE(std::equal(needle.begin(), needle.end(), info.gep.begin(), info.gep.end()));
    EXPECT_EQ(info.basicType, "unsigned long long");
}

TEST_F(IRModuleInterpreterTest, getStructFieldInfo__name) {
    struct BasicTypeInfo info = irx.getStructFieldInfo("thread", {"name"});
    std::vector<size_t> needle = {0, 4};
    EXPECT_TRUE(std::equal(needle.begin(), needle.end(), info.gep.begin(), info.gep.end()));
    EXPECT_EQ(info.basicType, "char");
    std::cout << info.size_in_bytes << std::endl;
    EXPECT_TRUE(info.size_in_bytes == 16);
}

TEST_F(IRModuleInterpreterTest, getStructFieldInfo__name_0) {
    struct BasicTypeInfo info = irx.getStructFieldInfo("thread", {"name", "0"});
    std::vector<size_t> needle = {0, 4, 0};
    EXPECT_TRUE(std::equal(needle.begin(), needle.end(), info.gep.begin(), info.gep.end()));
}

TEST_F(IRModuleInterpreterTest, set_int__cpu) {
    struct thread *thread = (struct thread *) Memory; // !!ATTENTION
    memset(thread, 0, sizeof(struct thread));
    // Ensure compatibility by compiling the gtests and test.ll with the same clang version and arguments
    // alternative: use llvm debug info
    

    int id = 42;
    int cpuVal = 0x55;
    uintptr_t ptrVal = 0xCCAAFFEE;
    
    irx.irx_struct_get("thread", Memory, {"id"}, &id, sizeof(int));
    EXPECT_EQ(id, thread->id);

    irx.irx_struct_set("thread", Memory, {"cpu"}, &cpuVal, sizeof(int));
    EXPECT_EQ(cpuVal, thread->cpu);

    irx.irx_struct_set("thread", Memory, {"ptr"}, &ptrVal, sizeof(uintptr_t));
    EXPECT_EQ(ptrVal, (uintptr_t) thread->ptr);
}

TEST_F(IRModuleInterpreterTest, get_int__cpu) {
    struct thread *thread = (struct thread *) Memory; // !!ATTENTION
    memset(thread, 0, sizeof(struct thread));
    // Ensure compatibility by compiling the gtests and test.ll with the same clang version and arguments
    // alternative: use llvm debug info

    thread->id = 42;
    thread->cpu = 0x55;
    thread->ptr = (void*) 0xCCAAFFEE;
    
    int id = 0;
    int cpuVal = 0;
    uintptr_t ptrVal = 0;

    irx.irx_struct_get("thread", Memory, {"id"}, &id, sizeof(int));
    EXPECT_EQ(id, thread->id);

    irx.irx_struct_get("thread", Memory, {"cpu"}, &cpuVal, sizeof(int));
    EXPECT_EQ(cpuVal, thread->cpu);

    irx.irx_struct_get("thread", Memory, {"ptr"}, &ptrVal, sizeof(uintptr_t));
    EXPECT_EQ(ptrVal, (uintptr_t) thread->ptr);
}

template<typename T, size_t N = 1>
void irx_struct_setget_test(IRModuleInterpreter &irx, const char *name, void *m, std::vector<std::string> path, T* expectedValue) {
    size_t size = sizeof(T) * N;
    T testedValue[N];
    irx.irx_struct_set(name, m, path, expectedValue, size);
    irx.irx_struct_get(name, m, path, testedValue, size);
    EXPECT_TRUE(memcmp(expectedValue, testedValue, size) == 0);
}

TEST_F(IRModuleInterpreterTest, setget_int__cpu) {
    memset(Memory, 0, sizeof(struct thread)); // FIXME remove thread type dependancy
    int cpuVal = 0x55;
    uintptr_t ptrVal = 0xCCAAFFEE;
    irx_struct_setget_test<int>(irx, "thread", Memory , {"cpu"}, &cpuVal);
    irx_struct_setget_test<uintptr_t>(irx, "thread", Memory , {"ptr"}, &ptrVal);
}

TEST_F(IRModuleInterpreterTest, set_string_array_element) {
    memset(Memory, 0, sizeof(struct thread)); // FIXME remove thread type dependancy
    char c = 0x55;
    irx_struct_setget_test(irx, "thread", Memory, {"name", "0"}, &c);
}

TEST_F(IRModuleInterpreterTest, setget_string) {
    memset(Memory, 0, sizeof(struct thread)); // FIXME remove thread type dependancy
    char name[16] = "0123456789ABCDE";
    irx_struct_setget_test<char, 16>(irx, "thread", Memory , {"name"}, name);
}

TEST_F(IRModuleInterpreterTest, setget_bool) {
    memset(Memory, 0, sizeof(struct thread)); // FIXME remove thread type dependancy
    bool running = true;
    irx_struct_setget_test(irx, "thread", Memory , {"running"}, &running);
}

TEST_F(IRModuleInterpreterTest, setget__unsigned_long_long_int) {
    memset(Memory, 0, sizeof(struct thread)); // FIXME remove thread type dependancy
    thread_id id = 0x100;
    irx_struct_setget_test(irx, "thread", Memory , {"id"}, &id);
    irx_struct_setget_test(irx, "thread", Memory , {"base", "id"}, &id);
}

TEST_F(IRModuleInterpreterTest, setget__int_array) {
    memset(Memory, 0, sizeof(struct thread)); // FIXME remove thread type dependancy
    int n[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    irx_struct_setget_test(irx, "thread", Memory , {"base", "n"}, &n);
    n[0] = 0x100;
    irx_struct_setget_test(irx, "thread", Memory , {"base", "n", "0"}, &n[0]);
}

TEST_F(IRModuleInterpreterTest, setget__int_array2) {
    memset(Memory, 0, sizeof(struct thread)); // FIXME remove thread type dependancy
    int n = 0x100;
    irx_struct_setget_test(irx, "thread", Memory , {"base", "cpus1", "0", "n"}, &n);
    irx_struct_setget_test(irx, "thread", Memory , {"base", "cpus2", "0", "n"}, &n);
    irx_struct_setget_test(irx, "thread", Memory , {"base", "cpus2", "1", "n"}, &n);
}

TEST_F(IRModuleInterpreterTest, setget__pointer) {
    memset(Memory, 0, sizeof(struct thread)); // FIXME remove thread type dependancy
    uintptr_t pended_on = 0x100;
    irx_struct_setget_test(irx, "thread", Memory , {"base", "pended_on"}, &pended_on);
}

TEST_F(IRModuleInterpreterTest, setget__union_linked_list) {
    memset(Memory, 0, sizeof(struct thread)); // FIXME remove thread type dependancy
    // head and next in union
    uintptr_t head = 0x200;
    uintptr_t next = 0;
    // prev and tail in union
    uintptr_t tail = 0;
    uintptr_t prev = 0x300;
    irx_struct_setget_test(irx, "thread", Memory , {"base", "join_waiters", "head"}, &head);
    irx_struct_setget_test(irx, "thread", Memory , {"base", "join_waiters", "prev"}, &prev);
    irx.irx_struct_get("thread", Memory, {"base", "join_waiters", "next"}, &next, sizeof(uintptr_t));
    irx.irx_struct_get("thread", Memory, {"base", "join_waiters", "tail"}, &tail, sizeof(uintptr_t));
    EXPECT_EQ(head, next);
    EXPECT_EQ(tail, prev);
}
