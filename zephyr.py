from __future__ import annotations
from dataclasses import dataclass, field
from typing import Annotated, Optional, Union, TypedDict
import ctypes
import pprint

from dataclasses import dataclass, fields, is_dataclass

from diff import diff, cmp

import logging
# logging.basicConfig(level=logging.NOTSET)
# logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("zephyr.py")

@dataclass
class VOID:
    pass

class ObjectMap(TypedDict):
    # map all object with there id to the ptr that was fetched
    id: id
    ptr: int

class ReverseObjectMap(TypedDict):
    ptr: int
    id: id

UInt8  = Annotated[int, ctypes.c_uint8]
UInt16 = Annotated[int, ctypes.c_uint16]
UInt32 = Annotated[int, ctypes.c_uint32]
UInt64 = Annotated[int, ctypes.c_uint64]
Int8   = Annotated[int, ctypes.c_int8]
Int16  = Annotated[int, ctypes.c_int16]
Int32  = Annotated[int, ctypes.c_int32]
Int64  = Annotated[int, ctypes.c_int64]

@dataclass
class ArraySize:
    value: int

@dataclass
class ctype:
    kind: str

@dataclass
class PTR:
    cls: type
    ptr: int = None
    
    cache: object = None # pointer to object. to detect this case and automaticallaly use a pointer for the object instead of a struct. maybe also need to manage a map of python id(object) to vvptr addresses to find the same objects and reuse them (or safe the vvptr as an annotation into the object itself.
    # only load structs from pointer if accessed
    
#     def __get__(self, instance, owner):
#         return self.cache
#     
#     def __set__(self, instance, value):
#         self.cache = value
    
    def __repr__(self):
        ptr = self.ptr if self.ptr is not None else 0
        return f"PTR({hex(ptr)})"



# class StructProxy:
#     def __init__(self, irx):
#         self.objectMap = ObjectMap()
#         self.reverseObjectMap = ReverseObjectMap()
#         self.irx = irx
#     
#     def get(self, obj, ptr: PTR = None):
#         assert(ptr is None or type(obj) == ptr.cls)
#         assert(obj.__ptr__.ptr is None or ptr is None)
#         
#         if obj.ptr is None and ptr is not None:
#             obj.ptr = ptr.ptr
#         
#         assert(obj.ptr is not None and obj.ptr != 0)
#         self.irx.get_struct(obj.__ptr__.cls.__name__, obj.ptr, obj)
#         self.objectMap[obj.ptr] = obj
#         self.reverseObjectMap[id(obj)] = obj.ptr
#     
#     def deref(self, ptr: PTR):
#         if ptr.ptr not in self.objectMap:
#             obj = ptr.cls()
#             self.get(obj, ptr)
#         
#         return self.objectMap[ptr.ptr]
#     
#     def put_cache(self, obj):
#         if is_dataclass(obj):
#             for m in fields(obj):
#                 if not m.name.startswith("__"):
#                     ptr = getattr(obj, m.name)
#                     if m.type == 'PTR':
#                         if ptr.cache is not None:
#                             new = ptr.cache
#                             ptr.cache = None
#                             if id(new) not in self.reverseObjectMap:
#                                 self.put(new)
#                             ptr.ptr = new.ptr
#                             
#                     elif is_dataclass(ptr):
#                         self.put_cache(ptr)
#         
#     def put(self, obj, deep=False):
#         if obj.ptr is None:
#             ptr = self.irx.structAlloc("struct." + obj.__ptr__.cls.__name__)
#             obj.ptr = ptr.vvptr
#             
#             self.objectMap[obj.ptr] = obj
#             self.reverseObjectMap[id(obj)] = obj.ptr
#         
#         self.put_cache(obj)
#         
#         if deep:
#             uploaded = [obj.ptr]
#             for m in fields(obj):
#                 if m.type == 'PTR' and not m.name.startswith("__"):
#                     ptr = getattr(obj, m.name)
#                     if ptr.ptr in self.objectMap:
#                         if ptr.ptr not in uploaded:
#                             uploaded += [ptr.ptr]
#                             self.put(self.deref(ptr))
#         
#         obj2 = obj.__ptr__.cls()
#         self.irx.set_struct(obj.__ptr__.cls.__name__, obj.ptr, obj)
#         self.irx.get_struct(obj.__ptr__.cls.__name__, obj.ptr, obj2)
#         
#         missmatches = diff(obj, obj2)
#         [logger.error(f"Missmatch: {m}") for m in missmatches]
#         assert(len(missmatches) == 0)
#     
#     def Global(self, cls, global_name: str):
#         vvptr = self.irx.irx_get_global_addr(global_name)        
#         return self.deref(PTR(cls, vvptr));


def SYS_DLIST_STATIC_INIT(vvptr):
    # alternative support python id and convert to vvptr later
    return _dnode(vvptr, vvptr)


@dataclass
class StructPointerTrait:
    @property
    def ptr(self) -> int:
        return self.__ptr__.ptr
    
    @property
    def vvptr(self) -> hex:
        return hex(self.__ptr__.ptr) if self.__ptr__.ptr != None else hex(0)
    
    @ptr.setter
    def ptr(self, value: int):
        self.__ptr__.ptr = value

# struct _dnode {
@dataclass
class _dnode(StructPointerTrait):
        __ptr__: PTR = field(default_factory=lambda: PTR(_dnode))
    # 	union {
    # 		struct _dnode *head; /* ptr to head of list (sys_dlist_t) */
        # head: Optional['_dnode'] = field(default=None)
        # head: Optional['_dnode']
        
        # head: Optional[Annotated[_dnode, PTR, ctype("void*")]] = None
        head: PTR = field(default_factory=lambda: PTR(_dnode))
        # head: Annotated[_dnode, PTR(None), ctype("void*")]
    # 		struct _dnode *next; /* ptr to next node    (sys_dnode_t) */
        # next: Annotated[PTR, _dnode, ctype("void*")] = field(default_factory=PTR)
        next: PTR = field(default_factory=lambda: PTR(_dnode))
    # 	};
    # 	union {
    # 		struct _dnode *tail; /* ptr to tail of list (sys_dlist_t) */
        tail: PTR = field(default_factory=lambda: PTR(_dnode))
    # 		struct _dnode *prev; /* ptr to previous node (sys_dnode_t) */
        prev: PTR = field(default_factory=lambda: PTR(_dnode))
    # 	};
    # };

    # struct _priq_rb {
    # 	struct rbtree tree;
    # 	int next_order_key;
    # };
    # 
    # 
    # /* Traditional/textbook "multi-queue" structure.  Separate lists for a
    #  * small number (max 32 here) of fixed priorities.  This corresponds
    #  * to the original Zephyr scheduler.  RAM requirements are
    #  * comparatively high, but performance is very fast.  Won't work with
    #  * features like deadline scheduling which need large priority spaces
    #  * to represent their requirements.
    #  */
    # struct _priq_mq {
    # 	sys_dlist_t queues[K_NUM_THREAD_PRIO];
    # 	unsigned long bitmask[PRIQ_BITMAP_SIZE];
    # };

    
    
        # def __init__(self, head=None, tail=None):
        #     self.__ptr__ = PTR(_dnode)
        #     self.head = PTR(_dnode)
        #     self.next = PTR(_dnode)
        #     self.tail = PTR(_dnode)
        #     self.prev = PTR(_dnode)
        #     # if head is None:
        #         # self.head = self
        #     #     self.next = None
        #     # if tail is None:
        #     #     self.tail = self
        #     #     self.prev = None
    
    # ptrs: PtrMap # map the pointer to a actuall value
    
    # _dirty: bool = False
        # __metadata__: StructMetadata = field(default_factory=lambda: StructMetadata(_dnode))
        
        # def ptr() -> int:
            # return self.__metadata__.ptr.ptr

# struct _timeout {
@dataclass
class _timeout(StructPointerTrait):
    __ptr__: PTR = field(default_factory=lambda: PTR(_timeout))
    # 	sys_dnode_t node;
    node: _dnode = field(default_factory=_dnode)
    # 	_timeout_func_t fn;
    fn: Int32 = field(default=0)
    # fn: PTR = field(default_factory=lambda: PTR(Int8))
    # #ifdef CONFIG_TIMEOUT_64BIT
    # 	/* Can't use k_ticks_t for header dependency reasons */
    # 	int64_t dticks;
    # #else
    # 	int32_t dticks;
    # #endif
    dticks: Int64 = field(default=0)
    # };

# struct rbnode {
@dataclass
class rbnode:
    # 	/** @cond INTERNAL_HIDDEN */
    # 	struct rbnode *children[2];
    children: Annotated[list[rbnode], ArraySize(2)]
    # 	/** @endcond */
    # };


# struct _thread_base {
@dataclass
class _thread_base(StructPointerTrait):
    __ptr__: PTR = field(default_factory=lambda: PTR(_thread_base))
    # %struct._dnode, %struct._wait_q_t*, i8, i8, %union.anon.2, i32, i8*, %struct._timeout
    # 
    # 	/* this thread's entry in a ready/wait queue */
    # 	union {
    # 		sys_dnode_t qnode_dlist;
    qnode_dlist: _dnode = field(default_factory=_dnode)
    # 		struct rbnode qnode_rb;
    qnode_rb: rbnode = None
    # 	};
    # 
    # 	/* wait queue on which the thread is pended (needed only for
    # 	 * trees, not dumb lists)
    # 	 */
    # 	_wait_q_t *pended_on;
    pended_on: PTR = field(default_factory=lambda: PTR(_dnode))
    # 
    # 	/* user facing 'thread options'; values defined in include/kernel.h */
    # 	uint8_t user_options;
    user_options: Int8 = 0
    # 
    # 	/* thread state */
    # 	uint8_t thread_state;
    thread_state: Int8 = 0
    # 
    # 	/*
    # 	 * scheduler lock count and thread priority
    # 	 *
    # 	 * These two fields control the preemptibility of a thread.
    # 	 *
    # 	 * When the scheduler is locked, sched_locked is decremented, which
    # 	 * means that the scheduler is locked for values from 0xff to 0x01. A
    # 	 * thread is coop if its prio is negative, thus 0x80 to 0xff when
    # 	 * looked at the value as unsigned.
    # 	 *
    # 	 * By putting them end-to-end, this means that a thread is
    # 	 * non-preemptible if the bundled value is greater than or equal to
    # 	 * 0x0080.
    # 	 */
    # 	union {
    # 		struct {
    # #ifdef CONFIG_BIG_ENDIAN
    # 			uint8_t sched_locked;
    # 			int8_t prio;
    # #else /* Little Endian */
    # 			int8_t prio;
    # 			uint8_t sched_locked;
    # #endif /* CONFIG_BIG_ENDIAN */
    # 		};
    # 		uint16_t preempt;
    preempt: Int16 = 0
    # 	};
    # 
    # #ifdef CONFIG_SCHED_DEADLINE
    # 	int prio_deadline;
    # #endif /* CONFIG_SCHED_DEADLINE */
    # 
    # 	uint32_t order_key;
    order_key: Int32 = 0
    # 
    # #ifdef CONFIG_SMP
    # 	/* True for the per-CPU idle threads */
    # 	uint8_t is_idle;
    # 
    # 	/* CPU index on which thread was last run */
    # 	uint8_t cpu;
    # 
    # 	/* Recursive count of irq_lock() calls */
    # 	uint8_t global_lock_count;
    # 
    # #endif /* CONFIG_SMP */
    # 
    # #ifdef CONFIG_SCHED_CPU_MASK
    # 	/* "May run on" bits for each CPU */
    # #if CONFIG_MP_MAX_NUM_CPUS <= 8
    # 	uint8_t cpu_mask;
    # #else
    # 	uint16_t cpu_mask;
    # #endif /* CONFIG_MP_MAX_NUM_CPUS */
    # #endif /* CONFIG_SCHED_CPU_MASK */
    # 
    # 	/* data returned by APIs */
    # 	void *swap_data;
    swap_data: Int32 = 0
    # 
    # #ifdef CONFIG_SYS_CLOCK_EXISTS
    # 	/* this thread's entry in a timeout queue */
    # 	struct _timeout timeout;
    timeout: _timeout = field(default_factory=_timeout)
    # #endif /* CONFIG_SYS_CLOCK_EXISTS */
    # 
    # #ifdef CONFIG_TIMESLICE_PER_THREAD
    # 	int32_t slice_ticks;
    # 	k_thread_timeslice_fn_t slice_expired;
    # 	void *slice_data;
    # #endif /* CONFIG_TIMESLICE_PER_THREAD */
    # 
    # #ifdef CONFIG_SCHED_THREAD_USAGE
    # 	struct k_cycle_stats  usage;   /* Track thread usage statistics */
    # #endif /* CONFIG_SCHED_THREAD_USAGE */
    # };


@dataclass
class _thread_arch:
    # a: Int32 = 0
    dummy: Int32 = 0

# %struct._callee_saved = type { i32, i32, i8* }
# posix
@dataclass
class _callee_saved:
    key: Int32 = 0
    retval: Int32 = 0
    thread_status: Int32 = 0 # PTR

# struct rbtree {
@dataclass
class rbtree:
    # 	/** Root node of the tree */
    # 	struct rbnode *root;
    root: rbnode
    # 	/** Comparison function for nodes in the tree */
    # 	rb_lessthan_t lessthan_fn;
    lessthan_fn: Int32
    # 	/** @cond INTERNAL_HIDDEN */
    # 	int max_depth;
    max_depth: Int32
    # #ifdef CONFIG_MISRA_SANE
    # 	struct rbnode *iter_stack[Z_MAX_RBTREE_DEPTH];
    # 	unsigned char iter_left[Z_MAX_RBTREE_DEPTH];
    # #endif
    # 	/** @endcond */
    # };

# struct _priq_rb {
@dataclass
class _priq_rb:
    # 	struct rbtree tree;
    tree: rbtree
    # 	int next_order_key;
    next_order_key: Int32
    # };

# typedef struct {
@dataclass
class _wait_q_t: #(StructPointerTrait):
    # __ptr__: PTR = field(default_factory=lambda: PTR(_wait_q_t))
    # 	struct _priq_rb waitq;
    # ifdef CONFIG_WAITQ_SCALABLE
    # waitq: _priq_rb
    
    # else
    
    # waitq: sys_dlist_t
    waitq: _dnode = field(default_factory=_dnode)
    
    # endif
    
    # } _wait_q_t;



# struct k_thread {
@dataclass
class k_thread(StructPointerTrait):
    __ptr__: PTR = field(default_factory=lambda: PTR(k_thread))
    # %struct._thread_base, %struct._callee_saved, i8*, %struct._wait_q_t, i32, %struct.k_heap*, %struct._thread_arch
    # 
    # 	struct _thread_base base;
    base: _thread_base = field(default_factory=_thread_base)
    # 
    # 	/** defined by the architecture, but all archs need these */
    # 	struct _callee_saved callee_saved;
    # callee_saved: _callee_saved = field(default_factory=_callee_saved)
    # 
    # 	/** static thread init data */
    # 	void *init_data;
    init_data: PTR = field(default_factory=lambda: PTR(VOID))
    # 
    # 	/** threads waiting in k_thread_join() */
    # 	_wait_q_t join_queue;
    join_queue: _wait_q_t = field(default_factory=_wait_q_t)
    # join_queue: _dnode = field(default_factory=_dnode)
    # 
    # #if defined(CONFIG_POLL)
    # 	struct z_poller poller;
    # #endif /* CONFIG_POLL */
    # 
    # #if defined(CONFIG_EVENTS)
    # 	struct k_thread *next_event_link;
    # 
    # 	uint32_t   events;
    # 	uint32_t   event_options;
    # 
    # 	/** true if timeout should not wake the thread */
    # 	bool no_wake_on_timeout;
    # #endif /* CONFIG_EVENTS */
    # 
    # #if defined(CONFIG_THREAD_MONITOR)
    # 	/** thread entry and parameters description */
    # 	struct __thread_entry entry;
    # 
    # 	/** next item in list of all threads */
    # 	struct k_thread *next_thread;
    # #endif /* CONFIG_THREAD_MONITOR */
    # 
    # #if defined(CONFIG_THREAD_NAME)
    # 	/** Thread name */
    # 	char name[CONFIG_THREAD_MAX_NAME_LEN];
    # #endif /* CONFIG_THREAD_NAME */
    # 
    # #ifdef CONFIG_THREAD_CUSTOM_DATA
    # 	/** crude thread-local storage */
    # 	void *custom_data;
    # #endif /* CONFIG_THREAD_CUSTOM_DATA */
    # 
    # #ifdef CONFIG_THREAD_USERSPACE_LOCAL_DATA
    # 	struct _thread_userspace_local_data *userspace_local_data;
    # #endif /* CONFIG_THREAD_USERSPACE_LOCAL_DATA */
    # 
    # #if defined(CONFIG_ERRNO) && !defined(CONFIG_ERRNO_IN_TLS) && !defined(CONFIG_LIBC_ERRNO)
    # #ifndef CONFIG_USERSPACE
    # 	/** per-thread errno variable */
    # 	int errno_var;
    # errno_var: Int32 = 0
    # #endif /* CONFIG_USERSPACE */
    # #endif /* CONFIG_ERRNO && !CONFIG_ERRNO_IN_TLS && !CONFIG_LIBC_ERRNO */
    # 
    # #if defined(CONFIG_THREAD_STACK_INFO)
    # 	/** Stack Info */
    # 	struct _thread_stack_info stack_info;
    # #endif /* CONFIG_THREAD_STACK_INFO */
    # 
    # #if defined(CONFIG_USERSPACE)
    # 	/** memory domain info of the thread */
    # 	struct _mem_domain_info mem_domain_info;
    # 
    # 	/**
    # 	 * Base address of thread stack.
    # 	 *
    # 	 * If memory mapped stack (CONFIG_THREAD_STACK_MEM_MAPPED)
    # 	 * is enabled, this is the physical address of the stack.
    # 	 */
    # 	k_thread_stack_t *stack_obj;
    # 
    # 	/** current syscall frame pointer */
    # 	void *syscall_frame;
    # #endif /* CONFIG_USERSPACE */
    # 
    # 
    # #if defined(CONFIG_USE_SWITCH)
    # 	/* When using __switch() a few previously arch-specific items
    # 	 * become part of the core OS
    # 	 */
    # 
    # 	/** z_swap() return value */
    # 	int swap_retval;
    # 
    # 	/** Context handle returned via arch_switch() */
    # 	void *switch_handle;
    # #endif /* CONFIG_USE_SWITCH */
    # 	/** resource pool */
    # 	struct k_heap *resource_pool;
    resource_pool: PTR = field(default_factory=lambda: PTR(VOID))
    # 
    # #if defined(CONFIG_THREAD_LOCAL_STORAGE)
    # 	/* Pointer to arch-specific TLS area */
    # 	uintptr_t tls;
    # #endif /* CONFIG_THREAD_LOCAL_STORAGE */
    # 
    # #ifdef CONFIG_DEMAND_PAGING_THREAD_STATS
    # 	/** Paging statistics */
    # 	struct k_mem_paging_stats_t paging_stats;
    # #endif /* CONFIG_DEMAND_PAGING_THREAD_STATS */
    # 
    # #ifdef CONFIG_PIPES
    # 	/** Pipe descriptor used with blocking k_pipe operations */
    # 	struct _pipe_desc pipe_desc;
    # #endif /* CONFIG_PIPES */
    # 
    # #ifdef CONFIG_OBJ_CORE_THREAD
    # 	struct k_obj_core  obj_core;
    # #endif /* CONFIG_OBJ_CORE_THREAD */
    # 
    # #ifdef CONFIG_SMP
    # 	/** threads waiting in k_thread_suspend() */
    # 	_wait_q_t  halt_queue;
    # #endif /* CONFIG_SMP */
    # 
    # 	/** arch-specifics: must always be at the end */
    # 	struct _thread_arch arch;
    # arch: _thread_arch = field(default_factory=_thread_arch)
    # };


# 
# struct _ready_q {
@dataclass
class _ready_q(StructPointerTrait):
    __ptr__: PTR = field(default_factory=lambda: PTR(_ready_q))
    # #ifndef CONFIG_SMP
    # 	/* always contains next thread to run: cannot be NULL */
    # 	struct k_thread *cache;
    cache: PTR = field(default_factory=lambda: PTR(k_thread))
    # #endif
    # 
    # #if defined(CONFIG_SCHED_DUMB)
    # 	sys_dlist_t runq;
    runq: _dnode = field(default_factory=_dnode)
    # #elif defined(CONFIG_SCHED_SCALABLE)
    # 	struct _priq_rb runq;
    # #elif defined(CONFIG_SCHED_MULTIQ)
    # 	struct _priq_mq runq;
    # #endif
    # };

@dataclass
class k_spinlock:
    # #ifdef CONFIG_SMP
    # #ifdef CONFIG_TICKET_SPINLOCKS
    #     /*
    #     * Ticket spinlocks are conceptually two atomic variables,
    #     * one indicating the current FIFO head (spinlock owner),
    #     * and the other indicating the current FIFO tail.
    #     * Spinlock is acquired in the following manner:
    #     * - current FIFO tail value is atomically incremented while it's
    #     *   original value is saved as a "ticket"
    #     * - we spin until the FIFO head becomes equal to the ticket value
    #     *
    #     * Spinlock is released by atomic increment of the FIFO head
    #     */
    #     atomic_t owner;
    #     atomic_t tail;
    # #else
    #     atomic_t locked;
    # #endif /* CONFIG_TICKET_SPINLOCKS */
    # #endif /* CONFIG_SMP */

    # #ifdef CONFIG_SPIN_VALIDATE
    #     /* Stores the thread that holds the lock with the locking CPU
    #     * ID in the bottom two bits.
    #     */
    #     uintptr_t thread_cpu;
    # #ifdef CONFIG_SPIN_LOCK_TIME_LIMIT
    #     /* Stores the time (in cycles) when a lock was taken
    #     */
    #     uint32_t lock_time;
    # #endif /* CONFIG_SPIN_LOCK_TIME_LIMIT */
    # #endif /* CONFIG_SPIN_VALIDATE */

    # #if defined(CONFIG_CPP) && !defined(CONFIG_SMP) && \
    #     !defined(CONFIG_SPIN_VALIDATE)
    #     /* If CONFIG_SMP and CONFIG_SPIN_VALIDATE are both not defined
    #     * the k_spinlock struct will have no members. The result
    #     * is that in C sizeof(k_spinlock) is 0 and in C++ it is 1.
    #     *
    #     * This size difference causes problems when the k_spinlock
    #     * is embedded into another struct like k_msgq, because C and
    #     * C++ will have different ideas on the offsets of the members
    #     * that come after the k_spinlock member.
    #     *
    #     * To prevent this we add a 1 byte dummy member to k_spinlock
    #     * when the user selects C++ support and k_spinlock would
    #     * otherwise be empty.
    #     */
    #     char dummy;
    # #endif
    pass

# 
# struct _cpu {
@dataclass
class _cpu(StructPointerTrait):
    __ptr__: PTR = field(default_factory=lambda: PTR(_cpu))
    # type { i32, i8*, %struct.k_thread*, %struct.k_thread*, i8, %struct.k_spinlock }
    # 	/* nested interrupt count */
    # 	uint32_t nested;
    nested: Int32 = 0
    # 
    # 	/* interrupt stack pointer base */
    # 	char *irq_stack;
    irq_stack: PTR = field(default_factory=lambda: PTR(VOID))
    # 
    # 	/* currently scheduled thread */
    # 	struct k_thread *current;
    current: PTR = field(default_factory=lambda: PTR(k_thread))
    # 
    # 	/* one assigned idle thread per CPU */
    # 	struct k_thread *idle_thread;
    idle_thread: PTR = field(default_factory=lambda: PTR(k_thread))
    # 
    # #ifdef CONFIG_SCHED_CPU_MASK_PIN_ONLY
    # 	struct _ready_q ready_q;
    # #endif
    # 
    # #if (CONFIG_NUM_METAIRQ_PRIORITIES > 0) &&                                                         \
    # 	(CONFIG_NUM_COOP_PRIORITIES > CONFIG_NUM_METAIRQ_PRIORITIES)
    # 	/* Coop thread preempted by current metairq, or NULL */
    # 	struct k_thread *metairq_preempted;
    # #endif
    # 
    # 	uint8_t id;
    id: Int8 = 0
    # 
    # #if defined(CONFIG_FPU_SHARING)
    # 	void *fp_ctx;
    # #endif
    # 
    # #ifdef CONFIG_SMP
    # 	/* True when _current is allowed to context switch */
    # 	uint8_t swap_ok;
    # #endif
    # 
    # #ifdef CONFIG_SCHED_THREAD_USAGE
    # 	/*
    # 	 * [usage0] is used as a timestamp to mark the beginning of an
    # 	 * execution window. [0] is a special value indicating that it
    # 	 * has been stopped (but not disabled).
    # 	 */
    # 
    # 	uint32_t usage0;
    # 
    # #ifdef CONFIG_SCHED_THREAD_USAGE_ALL
    # 	struct k_cycle_stats *usage;
    # #endif
    # #endif
    # 
    # #ifdef CONFIG_OBJ_CORE_SYSTEM
    # 	struct k_obj_core  obj_core;
    # #endif
    # 
    # 	/* Per CPU architecture specifics */
    # 	struct _cpu_arch arch;
    arch: k_spinlock = field(default_factory=k_spinlock) # probably just an empty struct, and not k_spinlock
    # };
# 
# struct z_kernel {
CONFIG_MP_MAX_NUM_CPUS = 1
# config.MP_MAX_NUM_CPUS = 1
@dataclass
class z_kernel(StructPointerTrait):
    __ptr__: PTR = field(default_factory=lambda: PTR(z_kernel))
    # { [1 x %struct._cpu], %struct._ready_q }
    # 	struct _cpu cpus[CONFIG_MP_MAX_NUM_CPUS];
    cpus: Annotated(list(_cpu), ArraySize(CONFIG_MP_MAX_NUM_CPUS)) = field(default_factory=lambda: [_cpu()] * CONFIG_MP_MAX_NUM_CPUS)
    # 
    # #ifdef CONFIG_PM
    # 	int32_t idle; /* Number of ticks for kernel idling */
    # #endif
    # 
    # 	/*
    # 	 * ready queue: can be big, keep after small fields, since some
    # 	 * assembly (e.g. ARC) are limited in the encoding of the offset
    # 	 */
    # #ifndef CONFIG_SCHED_CPU_MASK_PIN_ONLY
    # 	struct _ready_q ready_q;
    ready_q: _ready_q = field(default_factory=_ready_q)
    # #endif
    # 
    # #ifdef CONFIG_FPU_SHARING
    # 	/*
    # 	 * A 'current_sse' field does not exist in addition to the 'current_fp'
    # 	 * field since it's not possible to divide the IA-32 non-integer
    # 	 * registers into 2 distinct blocks owned by differing threads.  In
    # 	 * other words, given that the 'fxnsave/fxrstor' instructions
    # 	 * save/restore both the X87 FPU and XMM registers, it's not possible
    # 	 * for a thread to only "own" the XMM registers.
    # 	 */
    # 
    # 	/* thread that owns the FP regs */
    # 	struct k_thread *current_fp;
    # #endif
    # 
    # #if defined(CONFIG_THREAD_MONITOR)
    # 	struct k_thread *threads; /* singly linked list of ALL threads */
    # #endif
    # #ifdef CONFIG_SCHED_THREAD_USAGE_ALL
    # 	struct k_cycle_stats usage[CONFIG_MP_MAX_NUM_CPUS];
    # #endif
    # 
    # #ifdef CONFIG_OBJ_CORE_SYSTEM
    # 	struct k_obj_core  obj_core;
    # #endif
    # 
    # #if defined(CONFIG_SMP) && defined(CONFIG_SCHED_IPI_SUPPORTED)
    # 	/* Identify CPUs to send IPIs to at the next scheduling point */
    # 	atomic_t pending_ipi;
    # #endif
    # };



# 
# structs generated by generator-struct follow now
# 

# <0x21f475b0> = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "k_sem", file: <0x225ea090>, line: 3111, size: 128, elements: <0x21f63018>)
# %struct.k_sem.42 = type { %struct._wait_q_t.26, i32, i32 }
@dataclass
class k_sem(StructPointerTrait):
    __ptr__: PTR = field(default_factory=lambda: PTR(k_sem))
    # <0x21f44500> = !DIDerivedType(tag: DW_TAG_typedef, name: "_wait_q_t", file: <0x225e1310>, line: 286, baseType: <0x21f48830>)
    wait_q: _wait_q_t = field(default_factory=_wait_q_t)


    # <0x2245c298> = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
    count: UInt32 = 0


    # <0x2245c298> = !DIBasicType(name: "unsigned int", size: 32, encoding: DW_ATE_unsigned)
    limit: UInt32 = 0


"""
digraph{ 
"0x33f39fb0"[label="DW_TAG_typedef\nk_timeout_t"]
"0x33f48340"[label="DW_TAG_structure_type\n"]
"0x33f3c130"[label="DW_TAG_member\nticks"]
"0x33f3c130" -> "0x33f39cc0"
"0x33f48340" -> "0x33f3c130"
"0x33f39fb0" -> "0x33f48340"
} 
"""
# <0x33f39fb0> = !DIDerivedType(tag: DW_TAG_typedef, name: "k_timeout_t", file: <0x33f3b300>, line: 67, baseType: <0x33f48340>)
# %struct.k_timeout_t = type { i64 }
    # <0x33f39fb0> = !DIDerivedType(tag: DW_TAG_typedef, name: "k_timeout_t", file: <0x33f3b300>, line: 67, baseType: <0x33f48340>)
@dataclass
class k_timeout_t(StructPointerTrait):
    __ptr__: PTR = field(default_factory=lambda: PTR(k_timeout_t))
    # <0x33f39cc0> = !DIDerivedType(tag: DW_TAG_typedef, name: "k_ticks_t", file: <0x33f3b300>, line: 46, baseType: <0x33f3b570>)
    ticks: Int64 = 0


"""
digraph{ 
"0x33f44680"[label="DW_TAG_structure_type\nk_timer"]
"0x33f40da0"[label="DW_TAG_member\ntimeout"]
"0x33f40da0" -> "0x33f44a70"
"0x33f44680" -> "0x33f40da0"
"0x33f44680" -> "0x33f39bd0"
"0x33f3a180"[label="DW_TAG_member\nexpiry_fn"]
"0x33f3bb40"[label="DW_TAG_pointer_type\n"]
"0x33f21510"[label="DW_TAG_subroutine_type\n"]
"0x33f3bb40" -> "0x33f21510"
"0x33f3a180" -> "0x33f3bb40"
"0x33f44680" -> "0x33f3a180"
"0x33f38040"[label="DW_TAG_member\nstop_fn"]
"0x33f3bb40"[label="DW_TAG_pointer_type\n"]
"0x33f21510"[label="DW_TAG_subroutine_type\n"]
"0x33f3bb40" -> "0x33f21510"
"0x33f38040" -> "0x33f3bb40"
"0x33f44680" -> "0x33f38040"
"0x33f3a990"[label="DW_TAG_member\nperiod"]
"0x33f3a990" -> "0x33f39fb0"
"0x33f44680" -> "0x33f3a990"
"0x33f3bda0"[label="DW_TAG_member\nstatus"]
"0x33f3bda0" -> "0x33f3dc40"
"0x33f44680" -> "0x33f3bda0"
"0x33f23ba0"[label="DW_TAG_member\nuser_data"]
"0x33f3cbe0"[label="DW_TAG_pointer_type\n"]
"0x33f23ba0" -> "0x33f3cbe0"
"0x33f44680" -> "0x33f23ba0"
} 
"""
# <0x33f44680> = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "k_timer", file: <0x33f42490>, line: 1446, size: 416, elements: <0x33f24558>)
# %struct.k_timer = type { %struct._timeout, %struct._wait_q_t, void (%struct.k_timer*)*, void (%struct.k_timer*)*, %struct.k_timeout_t, i32, i8* }
@dataclass
class k_timer(StructPointerTrait):
    __ptr__: PTR = field(default_factory=lambda: PTR(k_timer))
    # <0x33f44a70> = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "_timeout", file: <0x33f42880>, line: 296, size: 160, elements: <0x33f3e718>)
    timeout: _timeout = field(default_factory=_timeout)


    # <0x33f42810> = !DIDerivedType(tag: DW_TAG_typedef, name: "_wait_q_t", file: <0x33f42880>, line: 286, baseType: <0x33f42940>)
    wait_q: _wait_q_t = field(default_factory=_wait_q_t)


    # <0x33f3bb40> = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: <0x33f21510>, size: 32)
    expiry_fn: int = 0

    # <0x33f3bb40> = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: <0x33f21510>, size: 32)
    stop_fn: int = 0


    # <0x33f39fb0> = !DIDerivedType(tag: DW_TAG_typedef, name: "k_timeout_t", file: <0x33f3b300>, line: 67, baseType: <0x33f48340>)
    period: k_timeout_t = field(default_factory=k_timeout_t)


    # <0x33f3dc40> = !DIDerivedType(tag: DW_TAG_typedef, name: "uint32_t", file: <0x33f3c840>, line: 26, baseType: <0x33f3c000>)
    status: UInt32 = 0


    # <0x33f3cbe0> = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: null, size: 32)
    user_data: int = 0



"""
digraph{ 
"0x38078bb0"[label="DW_TAG_structure_type\nk_mutex"]
"0x38078bb0" -> "0x38075770"
"0x38077740"[label="DW_TAG_member\nowner"]
"0x380779a0"[label="DW_TAG_pointer_type\n"]
"0x380779a0" -> "0x3807ee20"
"0x38077740" -> "0x380779a0"
"0x38078bb0" -> "0x38077740"
"0x38082640"[label="DW_TAG_member\nlock_count"]
"0x38082640" -> "0x3807ffb0"
"0x38078bb0" -> "0x38082640"
"0x38073430"[label="DW_TAG_member\nowner_orig_prio"]
"0x38073430" -> "0x38081d68"
"0x38078bb0" -> "0x38073430"
} 
"""
# <0x38078bb0> = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "k_mutex", file: <0x38078c10>, line: 2914, size: 160, elements: <0x38075540>)
# %struct.k_mutex = type { %struct._wait_q_t, %struct.k_thread*, i32, i32 }
@dataclass
class k_mutex(StructPointerTrait):
    __ptr__: PTR = field(default_factory=lambda: PTR(k_mutex))
    # <0x380759d0> = !DIDerivedType(tag: DW_TAG_typedef, name: "_wait_q_t", file: <0x38076690>, line: 286, baseType: <0x3807b6e0>)
    wait_q: _wait_q_t = field(default_factory=_wait_q_t)


    # <0x380779a0> = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: <0x3807ee20>, size: 32)
    owner: PTR = field(default_factory=lambda: PTR(k_thread))
    # <0x3807ffb0> = !DIDerivedType(tag: DW_TAG_typedef, name: "uint32_t", file: <0x3807f470>, line: 26, baseType: <0x38080030>)
    lock_count: UInt32 = 0


    # <0x38081d68> = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
    owner_orig_prio: Int32 = 0
    


"""
digraph{ 
"0x38083230"[label="DW_TAG_structure_type\nk_msgq"]
"0x3807d280"[label="DW_TAG_member\nwait_q"]
"0x3807d280" -> "0x380759d0"
"0x38083230" -> "0x3807d280"
"0x3807d300"[label="DW_TAG_member\nlock"]
"0x3807d300" -> "0x38082af0"
"0x38083230" -> "0x3807d300"
"0x3807d380"[label="DW_TAG_member\nmsg_size"]
"0x3807d380" -> "0x38082880"
"0x38083230" -> "0x3807d380"
"0x3807d400"[label="DW_TAG_member\nmax_msgs"]
"0x3807d400" -> "0x3807ffb0"
"0x38083230" -> "0x3807d400"
"0x3807d480"[label="DW_TAG_member\nbuffer_start"]
"0x3807d500"[label="DW_TAG_pointer_type\n"]
"0x3807d500" -> "0x38083038"
"0x3807d480" -> "0x3807d500"
"0x38083230" -> "0x3807d480"
"0x3807d580"[label="DW_TAG_member\nbuffer_end"]
"0x3807d500"[label="DW_TAG_pointer_type\n"]
"0x3807d500" -> "0x38083038"
"0x3807d580" -> "0x3807d500"
"0x38083230" -> "0x3807d580"
"0x3807d600"[label="DW_TAG_member\nread_ptr"]
"0x3807d500"[label="DW_TAG_pointer_type\n"]
"0x3807d500" -> "0x38083038"
"0x3807d600" -> "0x3807d500"
"0x38083230" -> "0x3807d600"
"0x38079f20"[label="DW_TAG_member\nwrite_ptr"]
"0x3807d500"[label="DW_TAG_pointer_type\n"]
"0x3807d500" -> "0x38083038"
"0x38079f20" -> "0x3807d500"
"0x38083230" -> "0x38079f20"
"0x3807ced0"[label="DW_TAG_member\nused_msgs"]
"0x3807ced0" -> "0x3807ffb0"
"0x38083230" -> "0x3807ced0"
"0x3807cfe0"[label="DW_TAG_member\nflags"]
"0x3807cfe0" -> "0x3807f3d0"
"0x38083230" -> "0x3807cfe0"
} 
"""
# <0x38083230> = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "k_msgq", file: <0x38078c10>, line: 4423, size: 320, elements: <0x3807d1a0>)
# %struct.k_msgq = type { %struct._wait_q_t, %struct.k_spinlock, i32, i32, i8*, i8*, i8*, i8*, i32, i8 }
@dataclass
class k_msgq(StructPointerTrait):
    __ptr__: PTR = field(default_factory=lambda: PTR(k_msgq))
    # <0x380759d0> = !DIDerivedType(tag: DW_TAG_typedef, name: "_wait_q_t", file: <0x38076690>, line: 286, baseType: <0x3807b6e0>)
    wait_q: _wait_q_t = field(default_factory=_wait_q_t)


    # <0x38082af0> = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "k_spinlock", file: <0x38082b50>, line: 45, elements: <0x38075f30>)
    lock: k_spinlock = field(default_factory=k_spinlock)


    # <0x38082880> = !DIDerivedType(tag: DW_TAG_typedef, name: "size_t", file: <0x380828f0>, line: 46, baseType: <0x38043c18>)
    msg_size: UInt32 = 0


    # <0x3807ffb0> = !DIDerivedType(tag: DW_TAG_typedef, name: "uint32_t", file: <0x3807f470>, line: 26, baseType: <0x38080030>)
    max_msgs: UInt32 = 0


    # <0x3807d500> = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: <0x38083038>, size: 32)
    buffer_start: PTR = field(default_factory=lambda: PTR(UInt8))
    # <0x3807d500> = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: <0x38083038>, size: 32)
    buffer_end: PTR = field(default_factory=lambda: PTR(UInt8))
    # <0x3807d500> = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: <0x38083038>, size: 32)
    read_ptr: PTR = field(default_factory=lambda: PTR(UInt8))
    # <0x3807d500> = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: <0x38083038>, size: 32)
    write_ptr: PTR = field(default_factory=lambda: PTR(UInt8))
    # <0x3807ffb0> = !DIDerivedType(tag: DW_TAG_typedef, name: "uint32_t", file: <0x3807f470>, line: 26, baseType: <0x38080030>)
    used_msgs: UInt32 = 0


    # <0x3807f3d0> = !DIDerivedType(tag: DW_TAG_typedef, name: "uint8_t", file: <0x3807f470>, line: 24, baseType: <0x3807f4f0>)
    flags: UInt8 = 0



@dataclass
class _snode(StructPointerTrait):
    __ptr__: PTR = field(default_factory=lambda: PTR(_snode))
    # struct _snode {
    # 	struct _snode *next;
    next: PTR = field(default_factory=lambda: PTR(_snode))
    # };

@dataclass
class _slist(StructPointerTrait):
    __ptr__: PTR = field(default_factory=lambda: PTR(_slist))
    # struct _slist {
        # sys_snode_t *head;
        # sys_snode_t *tail;
    # };
    head: PTR = field(default_factory=lambda: PTR(_snode))
    tail: PTR = field(default_factory=lambda: PTR(_snode))
    

@dataclass
class k_work_q(StructPointerTrait):
    __ptr__: PTR = field(default_factory=lambda: PTR(k_work_q))
    # struct k_work_q {
    # 	/* The thread that animates the work. */
    # 	struct k_thread thread;
    thread: k_thread = field(default_factory=k_thread)
    # 
    # 	/* All the following fields must be accessed only while the
    # 	 * work module spinlock is held.
    # 	 */
    # 
    # 	/* List of k_work items to be worked. */
    # 	sys_slist_t pending;
    pending: _slist = field(default_factory=_slist)
    # 
    # 	/* Wait queue for idle work thread. */
    # 	_wait_q_t notifyq;
    notifyq: _wait_q_t = field(default_factory=_wait_q_t)
    # 
    # 	/* Wait queue for threads waiting for the queue to drain. */
    # 	_wait_q_t drainq;
    drainq: _wait_q_t = field(default_factory=_wait_q_t)
    # 
    # 	/* Flags describing queue state. */
    # 	uint32_t flags;
    flags: UInt32 = 0
    # };


# struct k_work {
@dataclass
class k_work(StructPointerTrait):
    __ptr__: PTR = field(default_factory=lambda: PTR(k_work))
    # 	/* All fields are protected by the work module spinlock.  No fields
    # 	 * are to be accessed except through kernel API.
    # 	 */
    # 
    # 	/* Node to link into k_work_q pending list. */
    # 	sys_snode_t node;
    node: _dnode = field(default_factory=_dnode)
    # 
    # 	/* The function to be invoked by the work queue thread. */
    # 	k_work_handler_t handler;
    handler: PTR = field(default_factory=lambda: PTR(UInt8))
    # 	/* The queue on which the work item was last submitted. */
    # 	struct k_work_q *queue;
    queue: PTR = field(default_factory=lambda: PTR(k_work_q))
    # 
    # 	/* State of the work item.
    # 	 *
    # 	 * The item can be DELAYED, QUEUED, and RUNNING simultaneously.
    # 	 *
    # 	 * It can be RUNNING and CANCELING simultaneously.
    # 	 */
    # 	uint32_t flags;
    flags: UInt32 = 0
    # };
# 
# /** @brief A structure used to submit work after a delay. */
# struct k_work_delayable {
@dataclass
class k_work_delayable(StructPointerTrait):
    __ptr__: PTR = field(default_factory=lambda: PTR(k_work_delayable))
    # 	/* The work item. */
    # 	struct k_work work;
    node: work = field(default_factory=k_work)
    # 
    # 	/* Timeout used to submit work after a delay. */
    # 	struct _timeout timeout;
    timeout: _timeout = field(default_factory=_timeout)
    # 
    # 	/* The queue to which the work should be submitted. */
    # 	struct k_work_q *queue;
    queue: PTR = field(default_factory=lambda: PTR(k_work_q))
    # };


