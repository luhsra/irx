# README
```sh
myexports() {
export LLVM_PATH=/mnt/data/opt/llvm/14
export LLVM_PROJECT_SRC_PATH=/mnt/data/opt/llvm/llvm-project

# export LLVM_PATH=/srv/scratch/vit.fendel/llvm/14
# export LLVM_PROJECT_SRC_PATH=/srv/scratch/vit.fendel/llvm-project

export LD_LIBRARY_PATH=${LLVM_PATH}/lib
}

${LLVM_PATH}/bin/clang++ -S -g -emit-llvm test.cc
llvm-extract --func=print_max test.ll -o test_print_max.bc
llvm-dis test_print_max.bc

./irinterpreter2 -opaque-pointers -func print_max c test.ll


LLVM building external project:
-DLLVM_EXTERNAL_PROJECTS="Interpreter2" -DLLVM_EXTERNAL_INTERPRETER2_SOURCE_DIR=./llvm-subproject/Interpreter2


$(pidof irinterpreter2)
cat /proc/$(pidof irinterpreter2)/maps
info proc mappings
info file
info symbol 0x44ecf1



find 0x400000, 0x48f000, "t4"
find 0x7ffffffdc000, 0x7ffffffff000, "t4"



build() {
#meson setup builddir --native-file ../native.txt --prefix=$PWD/../ --wipe -Dtests=True
meson compile -C builddir
meson install -C builddir
}


# Unittest debugging
gdb -ex 'b process_composite_struct' -ex run --args ../builddir/tests/gtest_test --gtest_filter=IRModuleInterpreterTest.getStructFieldInfo__name






# DEBUGGING LLVM
(gdb) call LI.dump()
  %0 = load %struct.thread*, %struct.thread** %local_thread.addr, align 4, !dbg !90
(gdb) call PointerOperand->dump()
  %local_thread.addr = alloca %struct.thread*, align 4

```
