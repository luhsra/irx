// ${LLVM_PATH}/bin/clang++ -o main -Wall -Wl,-Map=main.map globals.ll test-main.c
// ${LLVM_PATH}/bin/clang++ -o test.elf -WL-Map=test.elf-working.map test.ll test-main.cc
// extract-blob native_sim-static_sys_sems-kernel.ll -o app.ll
// ${LLVM_PATH}/bin/clang++ -m32 -S -o test2.s /tmp/interpreter-composite.ll  -Wl,-Map=test2.map

extern "C" void run_native_tasks();

extern "C" void print(int a){}

int main() {
    run_native_tasks();
    return 0;
}
