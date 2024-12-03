#include "irinterpreter2-mmap.h"
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <list>
#include <map>
#include <utility>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <thread>
#include <sys/mman.h>

#include <llvm/Support/MemAlloc.h>

#include "irinterpreter2-struct.h"

static AllocationMap::iterator find_start(AllocationMap &m, uintptr_t k) {
    AllocationMap::iterator e = m.end();
    
    for(AllocationMap::iterator it = m.begin(); it != m.end(); ++it) {
        if(it->first <= k) {
            if(e == m.end() || e->first < it->first)
                e = it;
        }
    }
    
    return e;
}

static AllocationMap::iterator find_end(AllocationMap &m, uintptr_t k) {
    AllocationMap::iterator e = m.end();
    
    for(AllocationMap::iterator it = m.begin(); it != m.end(); ++it) {
        if(it->first >= k) {
            if(e == m.end() || e->first > it->first)
                e = it;
        }
    }
    
    return e;
}


bool IRXAllocator::has(uintptr_t vvptr) {
    AllocationMap::iterator start = find_start(vvmem_start, vvptr);
    AllocationMap::iterator end = find_end(vvmem_end, vvptr);
    
    return start != vvmem_start.end() && end != vvmem_end.end();
}

struct allocation IRXAllocator::find(uintptr_t vvptr) {
    AllocationMap::iterator start = find_start(vvmem_start, vvptr);
    AllocationMap::iterator end = find_end(vvmem_end, vvptr);
    
    // invalidate if no intersecting was found
    if(start->second != end->second) {
        start = vvmem_start.end();
        end = vvmem_end.end();
    }
    
    struct allocation_data *data = nullptr;
    
    if(start != vvmem_start.end())
        data = start->second;
    return {start, end, data};
};

void IRXAllocator::remove(uintptr_t vvptr) {
    auto alloc = find(vvptr);
    assert(alloc.start->second == alloc.end->second);

    void *vptr = alloc.start->second->vptr;
    vvmem_start.erase(alloc.start);
    vvmem_end.erase(alloc.end);
    allocationsHolder.remove_if([vptr](const struct allocation_data& alloc) -> bool { return alloc.vptr == vptr; });
}

// FIXME: maybe chose one and translate it later when used. what would rather make sense to return? vvptr or vptr?
struct irx_alloc IRXAllocator::put(uintptr_t vvptr, void *vptr, size_t size) {
    allocationsHolder.push_back({vptr, size});
    vvmem_start[vvptr] = &allocationsHolder.back();
    vvmem_end[vvptr + size] = &allocationsHolder.back();

    // std::memset(vptr, 0x00, size);

    return {vvptr, vptr, size};
}

void* IRXAllocator::vvptr2vptr(uintptr_t vvptr) {
    auto alloc = find(vvptr);
    
    if(alloc.data != nullptr) {
        assert(alloc.start->second == alloc.end->second);
        size_t offset = vvptr - alloc.start->first;
        return (void*)(((uintptr_t)alloc.data->vptr) + offset);
    }
    return nullptr;
}

AllocationMap::iterator IRXAllocator::find_vvptr(AllocationMap &vvmem_start, void *vptr) {
    return std::find_if(vvmem_start.begin(), vvmem_start.end(),
            [vptr](const std::pair<uintptr_t, struct allocation_data*> &haystack) -> bool {
            return haystack.second->vptr == vptr;
        });
}

uintptr_t IRXAllocator::vptr2vvptr(void *vptr) {
    AllocationMap::iterator alloc_data = find_vvptr(vvmem_start, vptr);
    
    if(alloc_data != vvmem_start.end()) {
        size_t offset = (uintptr_t)vptr - (uintptr_t)alloc_data->second->vptr;
        return alloc_data->first + offset;
    }
    return (uintptr_t) nullptr;
}

struct irx_alloc MMapAllocator::add(void *vptr, size_t size) {
    // for the mmap(MAP_32BIT) case vvmem == vmem
    // therefore we put the same address for vvptr and vptr
    uintptr_t vvptr = (uintptr_t) vptr;
    if((vvptr & ~0xFFFFFFFFLL)) {
        size += 1; // prevent mmap with size of zero
        std::cout << "Warning: trying to add a 64-bit address in MMapAllocator size:" << size << std::endl;
        void *m = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
        if (m == MAP_FAILED) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }
        extra_allocations.push_back({m, size}); // dummy allocation, just to reserve this address
        assert((vvptr & ~0xFFFFFFFFLL));
        
        vvptr = (uintptr_t) m;
    }
    return put(vvptr, vptr, size);
}

struct irx_alloc MMapAllocator::malloc(size_t size) {
    // if the target is 32 bit and our host systems is 64 bits the LLVM-IR is compiled to hold only 32 bit wide pointers.
    // a quick and dirty solution is to utilisize the mmap linux syscall.
    // The mmap syscall can be used to allocate 4k pages limited to the 32 bit address space
    // will allocate a page (4k), bad for small allocation, maybe: allocate a huge "RAM"-block, but would need to manage memory
    void *m = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    if (m == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    allocations.push_back(m);
    
    return add(m, size);
}

struct irx_alloc IRXAllocator::malloc(IRModuleInterpreter *irx, std::string name) {
    size_t size = irx->irx_struct_get_size(name.c_str());
    return this->malloc(size);
}

void MMapAllocator::free(uintptr_t vvptr) {
    auto alloc = find(vvptr);
    assert(alloc.start->second == alloc.end->second);
    
    if(isInternalAllocated(alloc.start->second->vptr)) {
        munmap(alloc.data->vptr, alloc.data->size);
        allocations.remove(alloc.start->second->vptr);
    }
    remove(vvptr);
}

bool IRXAllocator::isInternalAllocated(void* vptr) {
    return std::find(allocations.begin(), allocations.end(), vptr) != allocations.end();
}

IRXAllocator::~IRXAllocator() {};

LLVMGlobalVariableAllocator::~LLVMGlobalVariableAllocator() {
}

MapAllocator::~MapAllocator() {
    while(vvmem_start.size()) {
        auto alloc = vvmem_start.begin();
        this->free(alloc->first);
    }
}

MMapAllocator::~MMapAllocator() {
    while(vvmem_start.size()) {
        auto alloc = vvmem_start.begin();
        this->free(alloc->first);
    }
    
    for(auto const& alloc : extra_allocations) {
        munmap(alloc.vptr, alloc.size);
    }
}

uintptr_t LinearVirtualMemoryAddressGenerator::next(size_t size) {
    uintptr_t vvmem = lastAllocAddr;
    // lastAllocAddr += (size+3) & ~0x03; // align to 4 bytes (this is optional)
    lastAllocAddr += (size+4) & ~0x03; // make sure alloc of 0 will still get its own (unused) address
    
    if(lastAllocAddr > endAddr) {
        std::cerr << "Could not find free memory block for " << size << " bytes."
            << " Interpreter run out of virtual virtual memory, please increase memory size"
            << " or improve allocator algorithm."
            << " Current vvmem size is " << (vvsize >> 20) << "MB."
            << std::endl;
        exit(EXIT_FAILURE);
    }
    
    return vvmem;
}

struct irx_alloc MapAllocator::add(void *vmem, size_t size) {
    uintptr_t vvmem = vma.next(size);
    
    auto start = find(vvmem);
    auto end = find(vvmem + size + 1);
    
    if(start.start != vvmem_start.end() || end.start != vvmem_start.end()) {
        std::cerr << "Memory is address overlapping" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    return put(vvmem, vmem, size);
}


struct irx_alloc MapAllocator::malloc(size_t size) {
    void *vmem = llvm::safe_malloc(size);
    allocations.push_back(vmem);
    return add(vmem, size);
}

void MapAllocator::free(uintptr_t vvptr) {
    auto alloc = find(vvptr);
    assert(alloc.start->second == alloc.end->second);
    if(isInternalAllocated(alloc.start->second->vptr)) {
        ::free(alloc.data->vptr);
        allocations.remove(alloc.start->second->vptr);
    }
    remove(vvptr);
}


//thesis write about this: interpreter memory space: A, B, C
// A: use mmap to allocate pages:
// downsites: 4k bad for small allocation, possible improvments
// advantages: quick and dirty simple implementation, no need to change code, easy debugging
// B: downsites: most complex but advantages: os agnostic
// option C personality (unknown side effects, did not try)


struct irx_alloc LLVMGlobalVariableAllocator::malloc(IRModuleInterpreter *irx, std::string name) {
    llvm::StructType *Ty = llvm::StructType::getTypeByName(irx->Context, name);
    
    size_t size = irx->M->getDataLayout().getTypeStoreSize(Ty);
    
    std::ostringstream varname;
    static size_t n = 0;
    varname << "GV" << n++;
    
    unsigned ASIdx = irx->M->getDataLayout().getDefaultGlobalsAddressSpace();
    DBGS() << "ASIdx: " << ASIdx << "\n";
    
    llvm::GlobalVariable *GV = new llvm::GlobalVariable(Ty, false, llvm::GlobalValue::PrivateLinkage, nullptr, varname.str(), llvm::GlobalVariable::NotThreadLocal, ASIdx, false);
    // GlobalVariable *GV = new GlobalVariable(Ty, false, GlobalValue::PrivateLinkage, nullptr, varname.str(), GlobalVariable::NotThreadLocal, ASIdx, true);
    // or use getOrCreateGlobalVariable()
    
    auto GA = irx->I->getMemoryForGV(GV);
    
    // maybe it is possible to use the internal LLVM Global mapping, but it appears to be for named/"in ir code presend" global variables
    
//     irx->I->addGlobalMapping(varname.str(), (uint64_t) GA);
//     // I->updateGlobalMapping(GV, GA);
//     
//     void *ptr2 = irx->I->getPointerToGlobalIfAvailable(varname.str());
    
    return add(GA, size);
}

struct irx_alloc LLVMGlobalVariableAllocator::malloc(size_t size) {
    throw new std::runtime_error("unimplemented");
}

struct irx_alloc LLVMGlobalVariableAllocator::add(void *vmem, size_t size) {
    return put(vma.next(size), vmem, size);
}

void LLVMGlobalVariableAllocator::free(uintptr_t vvptr) {
    // need to free?: maybe call ->deleted() on GVMemoryBlock ?
    // GVMemoryBlock* b = vptr - sizeof(GVMemoryBlock);
    remove(vvptr);
}

// void* LLVMGlobalVariableAllocator::vvptr2vptr(uintptr_t vvptr) {
//     return (void*) vvptr;
// }
// 
// uintptr_t LLVMGlobalVariableAllocator::vptr2vvptr(void *vptr) {
//     return (uintptr_t) vptr;
// }

// this is just a simple linear allocator for the virtual virtual address.
LinearVirtualMemoryAddressGenerator::LinearVirtualMemoryAddressGenerator() {
    // we need to reserve memory addresses used by the interpreter for vvmem address space
    // so we don't accidentally translate addresses of llvm internal variables
    // therefore we can give out addresses to ther interpreter without worring about
    // address pointer accidentally overlapping with host-OS/interpreter allocated memory.
    
#ifdef DEBUG_MMAP
    void *m;
    for(size_t offset = 0; offset < 0xFF000000; offset += 0x00100000) { // retry until we find a free address range
        size_t addr = startAddr + offset;
        m = mmap((void*)addr, vvsize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (m == MAP_FAILED) {
            std::cerr << "mmap failed: " << std::hex << addr << std::dec << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        if((uintptr_t)m != startAddr)
            continue;
        
        startAddr = addr;
        break;
    }
    
    if((uintptr_t)m != startAddr) {
        std::cout << m << std::endl;
        std::cout << startAddr << std::endl;
    }
    assert((uintptr_t)m == startAddr);
#endif
}

void IRXAllocator::reset() {}
void IRXAllocator::freeze() {}

void MapAllocator::reset() {
    vma.reset();
}

void MapAllocator::freeze() {
    vma.freeze();
}

LinearVirtualMemoryAddressGenerator::~LinearVirtualMemoryAddressGenerator() {
#ifdef DEBUG_MMAP
    munmap((void*)startAddr, vvsize);
#endif
}

void LinearVirtualMemoryAddressGenerator::freeze() {
    freezeStack.push(lastAllocAddr);
}

void LinearVirtualMemoryAddressGenerator::reset() {
    lastAllocAddr = freezeStack.top();
    freezeStack.pop();
}


void* vvptr2vptr(IRXAllocator *allocator, uintptr_t vvptr) {
    void* vptr;
    assert(allocator);
    
    if(allocator->has(vvptr)) {
        vptr = allocator->vvptr2vptr(vvptr);
        DBGS() << blue << "Memory Map Translation Ptr: " << (void*)vvptr << " -> " << vptr << reset << "\n";
    } else {
        vptr = (void*) vvptr;
        DBGS() << blue << "Memory Map Translation Ptr: " << (void*)vvptr << " unchanged " << reset << "\n";
    }
    
    return vptr;
}

uintptr_t vptr2vvptr(IRXAllocator *allocator, void *vptr) {
    uintptr_t vvptr;
    assert(allocator);
    
    vvptr = allocator->vptr2vvptr(vptr);
    
    return vvptr;
}

uintptr_t MyInterpreter::vptr2vvptr(void *vptr) {
    return ::vptr2vvptr(allocator.get(), vptr);
}

void* MyInterpreter::vvptr2vptr(uintptr_t vvptr) {
    return ::vvptr2vptr(allocator.get(), vvptr);
}

void* IRModuleInterpreter::vvptr2vptr(uintptr_t vvptr) {
    return ::vvptr2vptr(I->allocator.get(), vvptr);
}

uintptr_t IRModuleInterpreter::vptr2vvptr(void *vptr) {
    assert(I->allocator);
    
    // if(allocator->has((uintptr_t)Val.PointerVal)) {
    //     void *vptr = allocator->vptr2vvptr((uintptr_t)Val.PointerVal);
    //     DBGS() << blue << "Memory Map Translation Val.PointerVal: " << Val.PointerVal << " -> " << vptr << "\n";
    //     Val.PointerVal = (GenericValue *) vptr;
    // } else {
    //     DBGS() << blue << "Memory Map Translation Val.PointerVal: " << Ptr << " unchanged " << "\n";
    //     vvptr = (uintptr_t) vptr;
    // }
    
    return I->allocator->vptr2vvptr(vptr);
}

struct irx_alloc MyInterpreter::malloc(IRModuleInterpreter *irx, std::string name) {
    struct irx_alloc alloc = allocator->malloc(irx, name);
    allocations.push_back(alloc.vvptr);
    return alloc;
}

struct irx_alloc MyInterpreter::malloc(size_t size) {
    struct irx_alloc alloc = allocator->malloc(size);
    allocations.push_back(alloc.vvptr);
    return alloc;
}
