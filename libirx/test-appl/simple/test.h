extern volatile int _extern_var;

typedef unsigned long long int thread_id;

struct _dnode {
	union {
		struct _dnode *head; /* ptr to head of list (sys_dlist_t) */
		struct _dnode *next; /* ptr to next node    (sys_dnode_t) */
	};
	union {
		struct _dnode *tail; /* ptr to tail of list (sys_dlist_t) */
		struct _dnode *prev; /* ptr to previous node (sys_dnode_t) */

		// __int128 test;
	};
};

struct _cpu {
    int n;
};

struct _thread_base {
	// union {
	// 	sys_dnode_t qnode_dlist;
	// 	struct rbnode qnode_rb;
	// };

    struct _cpu cpus1[1];
    struct _cpu cpus2[2];
    int n[8];
    
	struct _dnode *pended_on;

	struct _dnode join_waiters;
    
    thread_id id;
};

typedef struct _thread_base _thread_base_t;

struct k_thread {
	struct _thread_base base;
};

typedef struct k_thread _thread_t;
typedef struct k_thread *k_tid_t;

extern "C" struct /*__attribute__((packed))*/ thread {
    struct _thread_base base;
    thread_id id;
    bool running;
    int cpu;
    char name[16];
    void *ptr;
};
