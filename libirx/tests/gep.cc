#include <iostream>
#include "gtest/gtest.h"
#include <llvm/IR/Module.h>
#include "../irinterpreter2-struct.h"

using namespace std;
using namespace llvm;

class GEPTest : public testing::Test {
 protected:
  GEPTest() {
      EXPECT_TRUE(irx.parseIRFiles({"libirx/tests/dlist.ll"}));
  }
  
  ~GEPTest() override {}
  
  IRModuleInterpreter irx;
};

TEST_F(GEPTest, GEP2Member) {
    // std::list<struct TypedIndex> gep = irx.GEPIndices("k_thread", {"list"});
    std::list<size_t> gep = irx.GEP2Member("k_thread", "list");
    EXPECT_EQ(gep.size(), 1);
    auto it = gep.begin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(it, gep.end());
    
    auto needle = std::list<size_t> {1};
    EXPECT_TRUE(std::equal(needle.begin(), needle.end(), gep.begin(), gep.end()));
    
    gep = irx.GEP2Member("k_thread", "name");
    needle = std::list<size_t> {0, 0};
    EXPECT_TRUE(std::equal(needle.begin(), needle.end(), gep.begin(), gep.end()));
}

TEST_F(GEPTest, GEP) {
    std::list<size_t> gep;
    gep = irx.GEP("k_thread", {"list"});
    EXPECT_EQ(gep.size(), 2);
    auto it = gep.begin();
    EXPECT_EQ(*it++, 0);
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(it, gep.end());
    
    auto needle = std::list<size_t> {0, 1};
    EXPECT_TRUE(std::equal(needle.begin(), needle.end(), gep.begin(), gep.end()));
    
    gep = irx.GEP("k_thread", {"name"});
    needle = std::list<size_t> {0, 0, 0};
    EXPECT_TRUE(std::equal(needle.begin(), needle.end(), gep.begin(), gep.end()));
    
    gep = irx.GEP("k_thread", {"status"});
    needle = std::list<size_t> {0, 0, 0};
    EXPECT_TRUE(std::equal(needle.begin(), needle.end(), gep.begin(), gep.end()));
    gep = irx.GEP("k_thread", {"name", "0"});
    needle = std::list<size_t> {0, 0, 0, 0};
    EXPECT_TRUE(std::equal(needle.begin(), needle.end(), gep.begin(), gep.end()));
}

TEST_F(GEPTest, GEPArray) {
    std::list<size_t> gep;
    std::list<size_t> needle;
    
    gep = irx.GEP("k_thread", {"array"});
    needle = std::list<size_t> {0, 2};
    EXPECT_TRUE(std::equal(needle.begin(), needle.end(), gep.begin(), gep.end()));
    
    gep = irx.GEP("k_thread", {"array", "0"});
    needle = std::list<size_t> {0, 2, 0};
    EXPECT_TRUE(std::equal(needle.begin(), needle.end(), gep.begin(), gep.end()));
    
    gep = irx.GEP("k_thread", {"array", "1"});
    needle = std::list<size_t> {0, 2, 1};
    EXPECT_TRUE(std::equal(needle.begin(), needle.end(), gep.begin(), gep.end()));
    
    gep = irx.GEP("k_thread", {"array", "0", "next"});
    needle = std::list<size_t> {0, 2, 0, 0, 0};
    EXPECT_TRUE(std::equal(needle.begin(), needle.end(), gep.begin(), gep.end()));
    
    gep = irx.GEP("k_thread", {"array", "0", "next2"});
    needle = std::list<size_t> {0, 2, 0, 0, 0};
    EXPECT_TRUE(std::equal(needle.begin(), needle.end(), gep.begin(), gep.end()));
}

// TEST_F(GEPTest, getStructFieldInfo) {
//     // struct BasicTypeInfo getStructFieldInfo(std::string name, std::vector<std::string> _needle)
// }
// 

TEST_F(GEPTest, getStructFieldInfo__base_id) {
    struct BasicTypeInfo info = irx.getStructFieldInfo2("k_thread", {"name"});
    std::vector<size_t> needle = {0, 0, 0};
    EXPECT_TRUE(std::equal(needle.begin(), needle.end(), info.gep.begin(), info.gep.end()));
    EXPECT_EQ(info.basicType, "char");
}

TEST_F(GEPTest, getStructFieldInfo__name) {
    struct BasicTypeInfo info = irx.getStructFieldInfo("k_thread", {"name"});
    std::vector<size_t> needle = {0, 0, 0};
    EXPECT_TRUE(std::equal(needle.begin(), needle.end(), info.gep.begin(), info.gep.end()));
    EXPECT_TRUE(info.basicType.compare("char") == 0);
    std::cout << info.size_in_bytes << std::endl;
    EXPECT_EQ(info.size_in_bytes, 64);
}

// This test should not work currently
// TEST_F(GEPTest, getStructFieldInfo__name_0) {
//     // struct BasicTypeInfo info = irx.getStructFieldInfo("k_thread", {"name", "0"});
//     struct BasicTypeInfo info = irx.getStructFieldInfo2("k_thread", {"name", "0"});
//     std::vector<size_t> needle = {0, 0, 0, 0};
//     EXPECT_TRUE(std::equal(needle.begin(), needle.end(), info.gep.begin(), info.gep.end()));
//     EXPECT_EQ(info.basicType, "char");
// }

// TEST_F(GEPTest, getStructFieldInfo__name_02) {
//     // char m[256];
//     // char x = 0;
//     // irx.irx_struct_set("k_thread", m, {"name", "0"}, &x, 1);
// }

TEST_F(GEPTest, getOffset) {
    StructType *Ty = irx.getStructTypeByName("k_thread");
    // std::list<struct TypedIndex> indices = irx.GEPIndices("k_thread", std::list<std::string>{"name"});
    // std::list<struct TypedIndex> indices = irx.GEPIndices("k_thread", std::list<std::string>{"list"});
    std::list<struct TypedIndex> indices = irx.GEPIndices("k_thread", std::list<std::string>{"array", "0", "next"});
    size_t offset_bit = irx.GEP2Offset(*irx.M, Ty, indices);
    
    cout << "offset_bit: " << offset_bit << endl;
    
    EXPECT_EQ(offset_bit, 80 * 8);
    
    
    std::list<size_t> gep = irx.GEP("k_thread", {"array", "0", "next"});
    
}

