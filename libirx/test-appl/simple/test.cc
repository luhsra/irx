#define CONFIG_OPTION_A

#include "test.h"

struct thread *_current; // test global TODO
// TODO test linked list

struct thread global_thread_empty;
struct thread global_thread = {{}, 42, true, 15, "t4",  nullptr};

int n[128]; // TODO setter and getter for this

extern "C" void print(int a);
extern "C" void print(int a){};
// extern "C" void thread_swap(struct thread &);
extern "C" void thread_swap(struct thread &t) {
    // *((volatile int*)0x00) = t.id;
}

extern "C" int thread_sched(thread_id tid) {
    struct thread threads[] = {
        {{}, 1, false, 0, "t1", nullptr},
        {{}, 2, false, 0, "t2", nullptr},
        {{}, 3, false, 0, "t3", nullptr},
    };
    
    for(int i = 0; i < 3; i++) {
        struct thread &t = threads[i];
        if(t.id == tid) {
            thread_swap(t);
        }
    }
    
    return -1;
}

extern "C" int thread_join(struct thread *t) {
    thread_swap(*t);
    return 42;
}

extern "C" int thread_join2(struct thread t) {
    thread_swap(t);
    return 42;
}

extern "C" int print_max(int a, int b) {
#ifdef CONFIG_OPTION_A
    if(a > b) {
        print(a);
    } else {
        print(b);
    }
    return 1;
#endif
    return 2;
}

struct ptrw {
    struct ptrw *next;
};

extern "C" void call_thread_join(struct thread *local_thread, thread_id id) {
// extern "C" void call_thread_join() {
// extern "C" void call_thread_join(int a, int b) {
    // struct thread local_thread = {42, true, 15, "t4", nullptr};
    // local_thread = {42, true, 15, "t4", nullptr};
    
    // if(0xDEADCAFEUL == (unsigned long long)_current) {
    // if(local_thread == _current) {
    //     return;
    // }
    
    
//     struct ptrw p1, p2;
//     p1.next = &p2;
//     
//     p1.next->next = nullptr;
    
    // _extern_var = (int) p1.next;
    

    // _extern_var = 1;
    
    // struct thread *t = (struct thread *) local_thread->ptr;
    // if(t->id == id) {
    //     return;
    // }
    
    if(local_thread->id == id) {
        return;
    }
    

    // _extern_var = 2;
    // 
    thread_join2(*local_thread);
    // 
    // _extern_var = 3;
}

// test linker script variables referenced from zephyr code ...

#define __USED __attribute__((__used__))
#define __SECTION(x) __attribute__((__section__(x)))

extern "C" void printf(const char*, ...);

extern "C" void pre_boot43(void) { printf("%s\n", __func__); }
extern "C" void pre_boot42(void) { printf("%s\n", __func__); }

extern "C" void (* const pre_boot43_ptr)(void) __USED __SECTION(".native_PRE_BOOT_143_pre_boot") = pre_boot43;
extern "C" void (* const pre_boot42_ptr)(void) __USED __SECTION(".native_PRE_BOOT_142_pre_boot") = pre_boot42;

struct nsi_hw_event_st {
    int _dummy;
    int _dummy1;
    int _dummya[14];
};

static const struct nsi_hw_event_st __nsi_hw_event_3 __USED __SECTION(".nsi_hw_event_13") = {3, 6, {}};
static const struct nsi_hw_event_st __nsi_hw_event_2 __USED __SECTION(".nsi_hw_event_12") = {2, 4, {}};
static const struct nsi_hw_event_st __nsi_hw_event_1 __USED __SECTION(".nsi_hw_event_11") = {1, 2, {}};

static const struct nsi_hw_event_st k_semA __USED __SECTION("._k_sem.static.testA") = {1, 2, {}};
static const struct nsi_hw_event_st k_semB __USED __SECTION("._k_sem.static.testB") = {2, 4, {}};
static const struct nsi_hw_event_st k_semC __USED __SECTION("._k_sem.static.testC") = {3, 6, {}};

static volatile unsigned int number_of_events;

extern "C" struct nsi_hw_event_st __nsi_hw_events_start[];
extern "C" struct nsi_hw_event_st __nsi_hw_events_end[];

// extern "C" struct nsi_hw_event_st __nsi_hw_events_area[] = {__nsi_hw_event_1, __nsi_hw_event_2, __nsi_hw_event_3};
// extern "C" struct nsi_hw_event_st *__nsi_hw_events_start = &__nsi_hw_events_area[0];
// extern "C" struct nsi_hw_event_st *__nsi_hw_events_end = &__nsi_hw_events_area[2+1];

extern "C" void nsi_hws_init(void) {
    number_of_events = __nsi_hw_events_end - __nsi_hw_events_start;
    
    for(struct nsi_hw_event_st *start = __nsi_hw_events_start; start < __nsi_hw_events_end; ++start) {
        printf("dummy: %2d %2d\n", start->_dummy, start->_dummy1);
    }
}


// expected linker generated memory

// void (*native_area[])(void) = {pre_boot42, pre_boot43};
// void (**__native_PRE_BOOT_1_tasks_start)(void) = &native_area[0];
// void (**__native_tasks_end)(void) = &native_area[1+1];

// void (*native_area[])(void) = {pre_boot42, pre_boot43};
// void (*__native_PRE_BOOT_1_tasks_start[])(void) = {&native_area[0]};
// void (*__native_tasks_end[])(void) = {&native_area[1+1]};

extern "C" void run_native_tasks() {
    extern void (*__native_PRE_BOOT_1_tasks_start[])(void);
    extern void (*__native_tasks_end[])(void);
    
    printf("%0x\n", __native_PRE_BOOT_1_tasks_start);
    printf("%0x\n", __native_tasks_end);
    printf("%0x\n", __nsi_hw_events_start);
    printf("%0x\n", __nsi_hw_events_end);

    static void (**native_pre_tasks[])(void) = {
        __native_PRE_BOOT_1_tasks_start,
        __native_tasks_end
    };

    void (**fptr)(void);

    for (fptr = native_pre_tasks[0]; fptr < native_pre_tasks[1]; fptr++) {
        if (*fptr) {
            (*fptr)();
        }
    }
    
    nsi_hws_init();
    printf("%d\n", number_of_events);
}

