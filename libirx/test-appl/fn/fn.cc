#include <stdio.h>

volatile int x;

struct sa {
    int a;
};

struct sb {
    struct sa a;
};

struct big {
    int a;
    struct sa b;
    struct sa c;
    struct sb d;
    char s[1024];
};

extern "C" struct timeout {
    void (*fn)();
};

struct big big;
struct timeout t;

extern "C" void print() {
    x++;
    t.fn = (void (*)()) 0x12345678;
}


extern "C" void init(void) {
    t.fn = print;
}

int main() {
    init();
    t.fn();
    return 0;
}
