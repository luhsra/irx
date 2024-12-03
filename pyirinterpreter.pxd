# distutils: language=c++
# cython: language_level=3

from libcpp cimport bool
from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp.vector cimport list
from libc cimport stdint

cdef extern from "libirx/irinterpreter2-mmap.h":
    cdef struct irx_alloc:
        stdint.uintptr_t vvptr
        void* vptr
        size_t size
    
cdef extern from "libirx/irinterpreter2-struct.h":
    cdef struct BasicTypeInfo:
        string basicType
        stdint.uint64_t size_in_bits
        stdint.uint64_t size_in_bytes
        vector[stdint.uint64_t] gep
        
    cdef cppclass IRModuleInterpreter:
        IRModuleInterpreter()
        bool parseIRFiles(vector[string] InputFilenames, string name)
        bool createInterpreter()
        bool runFunction(string FunctionName, vector[llvmGenericValue] Args)
        bool resume()
        # stdint.uintptr_t structAlloc(string name)
        # void* structAlloc(string name)
        irx_alloc structAlloc(string name)
        
        int getPointerSize()
        
        void* vvptr2vptr(stdint.uint64_t vvptr);
        stdint.uint64_t vptr2vvptr(void *vptr);
        
        # structInfo(string name)
        
        void irx_struct_set(const char *name, void *m, vector[string] field, void *value, size_t size)
        void irx_struct_get(string name, void *m, vector[string], void *value, size_t len)
        
        void irx_set_global(const char *name, void *Memory, size_t size)
        void* irx_get_global_addr(const char *name);
        llvmGenericValue irx_get_global_var(const char *name)
        bool irx_create_global(string name, string typeName)
        size_t irx_struct_get_size(const char* name)
        size_t offsetInBitsFromPath(const char *name, vector[string] path)
        bool setInitializer(string name, string typeName)
        bool hasExternal(string name)
        vector[string] getDeclaredFunction()
        bool isFunctionDeclared(string name)
    
        BasicTypeInfo getStructFieldInfo(string name, vector[string] needle)
        BasicTypeInfo getStructFieldInfo2(string name, vector[string] needle)
        
        string findFunctionByVVPtr(stdint.uintptr_t vvptr)
        stdint.uint64_t FindFunctionNamed(string FunctionName)
        string getGlobalValueAtAddress(stdint.uintptr_t vvptr)
        
        void resetExtraAllocations()
        
        
        void stop(vector[string] functions)
        void skip(vector[string] functions)
        void setPrintLLVMIR(bool printit)
        vector[llvmGenericValue] getCurrentCallArgs()
        llvmGenericValue getExitValue()
        vector[string] getCallStack();
        string getCurrentCallFunctionName()
        bool isRunnable()
        void putReturnValue(llvmGenericValue Result)

# llvm

cdef extern from "llvm/ADT/APInt.h" namespace "llvm":
    cdef cppclass APInt:
        APInt()
        APInt(unsigned numBits, stdint.uint64_t val, bool isSigned)
        stdint.uint64_t getZExtValue()
        stdint.int64_t getSExtValue()

cdef extern from "llvm/ExecutionEngine/GenericValue.h":
    cdef cppclass llvmGenericValue "llvm::GenericValue":
        llvmGenericValue()
        llvmGenericValue(void* ptr)
        APInt IntVal
        double DoubleVal
        float FloatVal
        void* PointerVal
