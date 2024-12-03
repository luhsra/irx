from __future__ import annotations
from dataclasses import dataclass
from pyirinterpreter import PyIRModuleInterpreter
import struct
import pprint

from diff import diff, cmp

import zephyr

def create_thread():
    t = zephyr.k_thread()

    t.join_queue.waitq.head.cache = t
    t.join_queue.waitq.tail.cache = t
    t.base.qnode_dlist.head.cache = t
    t.base.qnode_dlist.tail.cache = t

    proxy.put(t)
    
    return t
    

    t1.join_queue.waitq.tail = thread_ptr.vvptr
    t1.join_queue.waitq.head = thread_ptr.vvptr

    # base.qnode_dlist = _dnode(thred_ptr.qnode_dlist, None, thred_ptr.qnode_dlist, None)
    t1.base.qnode_dlist = zephyr._dnode(thread_ptr.vvptr, None, thread_ptr.vvptr, None)

    # thread* next_up(): if nothing is queued will return idle thread

    # value = struct.pack('I', 0xFFFFFFFF)

    # print(hex(id(value)))
    # print(value)

    # irm.irx_struct_set(b"thread", thread_ptr, b"id", value, 4)


    irm.set_struct("k_thread", thread_ptr.vvptr, t1)
    # irm.set_struct("_thread_base", t1_base.vptr, base)
    # raise SystemExit

    # TODO
    # #define _current_cpu ({ __ASSERT_NO_MSG(!z_smp_cpu_mobile()); \
    # 			arch_curr_cpu(); })
    # #define _current k_sched_current_thread_query()
    # 
    # #else
    # #define _current_cpu (&_kernel.cpus[0])
    # #define _current _kernel.cpus[0].current
    # #endif
    # irm.irx_global_set_ptr("_current #symbol is a #define", thread_ptr)
    
    
    return thread_ptr


# slice_timeouts = [_timeout(
#     qnode_dlist = _dnode(0, None, 0, None)
#     # 	_timeout_func_t fn;
#     fn = 0x00
#     # #ifdef CONFIG_TIMEOUT_64BIT
#     # 	/* Can't use k_ticks_t for header dependency reasons */
#     # 	int64_t dticks;
#     dticks = 0
# )]


# z_kernel.ready_q.next = irm.structAlloc("struct._dnote")


# g = bytes((ctypes.c_char*16).from_address(_kernel_ptr))
# pprint.pp(g)  

def create_pyz_kernel(_kernel, current_thread, idle_thread):
#     runq = zephyr._dnode(0, None, 0, None)
#     # runq = zephyr._dnode(id(runq), None, id(runq), None) # maybe support id() in python, and the convert to pointer
# 
#     _kernel = zephyr.z_kernel(
#         cpus = [zephyr._cpu(nested = 0,
#                         irq_stack = 0,
#                         # current = 0,
#                         # current = t1, # meybe support like this (python style), but for now just set the address
#                         
#                         # PTR type shoudl only expand if it is accessed, and just have the pointer, and if accessed it will fetch tha actual object
#                         
#                         current = current_thread.__ptr__,
#                         # current = 0x54535251,
#                         # current = 0xFFFFFFFFFFF0000,
#                         idle_thread = idle_thread.__ptr__,
#                         # idle_thread = 0xD4D3D2D1, # debug value ....
#                         # idle_thread = 0x00,
#                         id = 0,
#                         arch = None)],
#         ready_q = zephyr._ready_q(cache=0, runq=runq),
#     )

    
    
    _kernel.cpus[0].current.ptr = current_thread.ptr
    _kernel.cpus[0].idle_thread.ptr = idle_thread.ptr
    # _kernel.ready_q.runq.head.cache = _kernel
    # _kernel.ready_q.runq.tail.cache = _kernel
    _kernel.ready_q.runq.head.ptr = 0
    _kernel.ready_q.runq.tail.ptr = 0
        
    return _kernel


class Thread(zephyr.StructProxy):
    def __init__(self):
        thread = zephyr.k_thread()
        super(Thread, self).__init__("struct.k_thread", thread)

class Global(zephyr.StructProxy):
    def __init__(self, struct_name, global_name):
        _kernel = create_pyz_kernel(0, 0)
        ptr = irm.irx_get_global_addr(global_name)
        super(Global, self).__init__(struct_name, _kernel, ptr)

irm = PyIRModuleInterpreter()

paths = ["libirx/zephyr.elf.ll"]
# paths = [b"../test.ll", b"../test-extern.ll"]
functionName = b"z_impl_k_thread_join"

if not irm.parseIRFiles(paths):
    raise FileNotFoundError(f"Failed to parse LLVM-IR file {str(paths)}")

if not irm.createInterpreter():
    raise RuntimeError("Failed to create interpreter")

# pprint.pp(irm.getDeclaredFunction())

# z_kernel = Global(class(zephyr.z_kernel), "_kernel")
# z_kernel = Global("z_kernel", "_kernel")

# print(z_kernel.ready_q.cache)

# _kernel = zephyr.z_kernel()

if irm.hasExternal("nsi_simu_time"):
    irm.setInitializer("nsi_simu_time", "i64 0")

# timeout_list = get_global(zephyr._dnode, "timeout_list")

proxy = zephyr.StructProxy(irm)
timeout_list = proxy.Global(zephyr._dnode, "timeout_list")


assert(timeout_list.head == timeout_list.tail)
assert(id(timeout_list) == id(proxy.deref(timeout_list.head)))

# timeout_list.head.cache = timeout_list
# print(timeout_list.head)
# timeout_list.tail = timeout_list
# 
# proxy.put(timeout_list)
# proxy.get(timeout_list)
# 
timeout_list_2 = proxy.Global(zephyr._dnode, "timeout_list")

assert(id(timeout_list) == id(timeout_list_2))



idle_thread = create_thread()
threads = [create_thread(), create_thread()]
_kernel = proxy.Global(zephyr.z_kernel, "_kernel")
_kernel = create_pyz_kernel(_kernel, threads[1], idle_thread)
pprint.pp(threads)
pprint.pp(_kernel)
proxy.put(_kernel)
proxy.get(_kernel)

print("_kernel.cpus[0].current: ", hex(_kernel.cpus[0].current.ptr))
print(proxy.deref(_kernel.cpus[0].current))


# slice_timeouts_ptr = irm.irx_get_global_addr("slice_timeouts") # todo handle arrays correctly ....
# irm.set_struct("_timeout", slice_timeouts_ptr, slice_timeouts)


# irqs_locked = irm.irx_get_global_addr("irqs_locked")
# print(hex(irqs_locked))
# irm.irx_global_set("irqs_locked", bytes([0]))
# raise SystemExit

# static uint64_t curr_tick;
# 
# static sys_dlist_t timeout_list = SYS_DLIST_STATIC_INIT(&timeout_list);
# 
# static struct k_spinlock timeout_lock;
# 
# #define MAX_WAIT (IS_ENABLED(CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE) \
# 		  ? K_TICKS_FOREVER : INT_MAX)
# 
# /* Ticks left to process in the currently-executing sys_clock_announce() */
# static int announce_remaining;
# 
# #if defined(CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME)
# int z_clock_hw_cycles_per_sec = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;


# timeout_list_vvptr = irm.irx_get_global_addr("timeout_list")
# print("timeout_list_vvptr:", hex(timeout_list_vvptr))
# timeout_list = SYS_DLIST_STATIC_INIT(timeout_list_vvptr)
# timeout_list2 = SYS_DLIST_STATIC_INIT(0)
# irm.set_struct("_dnode", timeout_list_vvptr, timeout_list)
# irm.get_struct("_dnode", timeout_list_vvptr, timeout_list2)
# if not cmp(timeout_list, timeout_list2):
#     raise RuntimeError

# ptr = create_primitive('int32_t') #maybe used for the global or args
# ptr = create_struct('struct.thread')
# ptr = create_global('struct.thread')

print(hex(threads[0].ptr))
args = [threads[0].__ptr__.ptr, 42]

irm.stop(["arch_swap"])

if not irm.runFunction(functionName, args, args_type_hints=['PTR', 'APInt64']):
    raise RuntimeError(f"Failed to run LLVM Interpreter for function {str(functionName)}")


if irm.getCurrentCallFunctionName() == "arch_swap":
    print(irm.getCallStack())
    print(irm.getCurrentCallFunctionName())
    pprint.pp(irm.getCurrentCallArgs())
    for old in [idle_thread, _kernel] + threads:
        print("\n", type(old), old.__ptr__)
        new = type(old)()
        new.ptr = old.ptr
        proxy.get(new)
        changes = diff(old, new)
        [print(f"{x.path_str():30s}: {x.obj1} => {x.obj2}") for x in changes]

while irm.isRunnable():
    if not irm.resume():
        raise RuntimeError(f"Failed to run LLVM Interpreter for function {str(functionName)}")

print("ExitValue:", irm.getExitValue())

raise SystemExit


# print(cmp(_kernel, _kernel2))


next_thread = create_pythread()

print(diff(_kernel, _kernel_new))

for t in threads + [idle_thread]:
    if t.vvptr == _kernel_new.ready_q.cache:
        irm.get_struct("k_thread", t.vvptr, next_thread)
        print("thread:", hex(t.vvptr))
        pprint.pp(next_thread)
