#include <stdio.h>

struct dlist {
    union {
        struct dlist *next;
        struct dlist *next2;
    };
    union {
        struct dlist *prev;
        struct dlist *prev2;
    };
};

typedef struct dlist sys_dlist;

struct k_thread {
    union {
        char name[64];
        int status;
    };
    sys_dlist list;
    sys_dlist array[2];
};

void add(struct dlist &a, struct dlist &b) {
    a.next = &b;
    b.prev = &a;
}

extern "C" int print(struct dlist *a) {
    int count = 0;
    while(a) {
        // printf("%8p\n", (void*) a);
        ++count;
        a = a->next;
    }
    
    return count;
}

struct k_thread t[3];

void init(void) {
    t[0].list.prev = 0;
    t[2].list.next = 0;
    
    add(t[0].list, t[1].list);
    add(t[1].list, t[2].list);
}

int main() {
    init();
    int ret = print(&t[0].list);
    printf("%d\n", ret);
    return 0;
}
