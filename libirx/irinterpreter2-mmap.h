#pragma once
#include <stddef.h>
#include <stdint.h>
#include <list>
#include <map>
#include <string>
#include <stack>

class IRModuleInterpreter;

// vvmem: memory in interpreter address space
// vmem: memory in host application address space

struct allocation_data {
    void* vptr; // host application address space
    size_t size;
    // bool internal; // true if memory is managed by the allocator, thus needs to be freed
};

typedef std::map<uintptr_t, struct allocation_data*> AllocationMap;

struct allocation {
    AllocationMap::iterator start;
    AllocationMap::iterator end;
    struct allocation_data *data;
    //uintptr_t vvmem; // interpreter address space // implicit avaialable from map key
};

struct irx_alloc {
    uintptr_t vvptr;
    void* vptr;
    size_t size;
};

class IRXAllocator {
protected:
    std::list<struct allocation_data> allocationsHolder;
    AllocationMap vvmem_start;
    AllocationMap vvmem_end;
    
    // a list of allocations that are internally managed, and need to be freed.
    std::list<void*> allocations;

    AllocationMap::iterator find_vvptr(AllocationMap &vvmem_start, void *vptr);

public:
    virtual ~IRXAllocator();
    virtual struct allocation find(uintptr_t vvptr);
    virtual struct irx_alloc malloc(size_t size) = 0;
    virtual struct irx_alloc malloc(IRModuleInterpreter *irx, std::string name);
    virtual void free(uintptr_t vvptr) {};
    
    virtual void* vvptr2vptr(uintptr_t vvptr);
    virtual uintptr_t vptr2vvptr(void *vptr);
    
    // could be private
    virtual struct irx_alloc put(uintptr_t vvptr, void *vptr, size_t size);
    virtual struct irx_alloc add(void *vptr, size_t size) = 0;
    virtual void remove(uintptr_t vvptr);
    virtual void freeze();
    virtual void reset();
    
    virtual bool isInternalAllocated(void* vptr);
    
    bool has(uintptr_t vvptr);
};

class MMapAllocator : public IRXAllocator {
    std::list<struct allocation_data> extra_allocations;
public:
    virtual ~MMapAllocator();
    virtual struct irx_alloc malloc(size_t size);
    virtual struct irx_alloc add(void *vptr, size_t size);
    virtual void free(uintptr_t vvptr);
};


// alternative C: personality
// #include <sys/personality.h>
//
// if(personality(ADDR_LIMIT_32BIT | ADDR_NO_RANDOMIZE) == -1) {
//        perror("mmap");
//        exit(EXIT_FAILURE);
// }
// class PersonalityAllocator : public IRXAllocator {
// };

class LinearVirtualMemoryAddressGenerator {
public:
    uintptr_t vvsize = 0x08000000; // 128MB
    uintptr_t startAddr = 0x50000000;
    uintptr_t endAddr = startAddr + vvsize;
    uintptr_t lastAllocAddr = startAddr;
    
    uintptr_t next(size_t size);
    
    std::stack<uintptr_t> freezeStack;
    
    void freeze();
    void reset();

    
    LinearVirtualMemoryAddressGenerator();
    virtual ~LinearVirtualMemoryAddressGenerator();
};

// alternative B:
// TODO FUTURE: a better way would be to actually manage the allocations in a 32bit to 64bit memory map
class MapAllocator : public IRXAllocator {
public:
    LinearVirtualMemoryAddressGenerator vma;
    virtual ~MapAllocator();
    virtual struct irx_alloc malloc(size_t size);
    virtual struct irx_alloc add(void *vptr, size_t size);
    virtual void free(uintptr_t vvptr);
    virtual void freeze();
    virtual void reset();
};




class LLVMGlobalVariableAllocator : public IRXAllocator {
    LinearVirtualMemoryAddressGenerator vma;
public:
    virtual ~LLVMGlobalVariableAllocator();
    virtual struct irx_alloc malloc(size_t size);
    virtual struct irx_alloc malloc(IRModuleInterpreter *irx, std::string name);
    virtual struct irx_alloc add(void *vptr, size_t size);
    virtual void free(uintptr_t vvptr);
    
    // virtual void* vvptr2vptr(uintptr_t vvptr);
    // virtual uintptr_t vptr2vvptr(void *vptr);
};
