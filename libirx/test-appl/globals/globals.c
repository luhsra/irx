void fn(void) {}

void (*fn7[])(void) = {fn, fn, fn, fn, fn, fn};

int main(void) {
    int ret = 0;
    for(int i = 0; i < 6; ++i) {
        ret |= fn == fn7[i];
    }
    return ret;
}
