# README
```sh

export CC="ccache gcc"
export CXX="ccache g++"
bear -- python3 setup.py build_ext -L${LD_LIBRARY_PATH} --inplace -j8

gdb -ex 'add-synbol-file pyirinterpreter.cpython-311-x86_64-linux-gnu.so' -ex run --args python3 run_test.py

cygdb . -- --args python3 -m pdb run_test.py





meson setup builddir --native-file native.txt --prefix=$PWD

build() {
    meson compile -C builddir
    meson install -C builddir
}
build() {
    meson compile -C ../..
    meson install -C ../..
}
meson test -C builddir -v gtest

export PYTHONPATH=lib64/python3.11/site-packages
export PYTHONPATH=lib/python3/dist-packages

python3 run_test.py
python3 -m unittest

gdb -ex run --args python run_test_zephyr.py


# zephyr simulation runner library
export LLVM_PATH=/mnt/data/opt/llvm/14
export LLVM_COMPILER_PATH=${LLVM_PATH}/bin/



NSI_CC=gclang NSI_AR=${LLVM_PATH}/bin/llvm-ar NSI_PATH=/mnt/data/Studium/Master_Thesis/Search/SRA/parrot/subprojects/zephyr/scripts/native_simulator/ NSI_BUILD_PATH=/tmp/native_simulator_build NSI_NATIVE=1 make -C /mnt/data/Studium/Master_Thesis/Search/SRA/parrot/subprojects/zephyr/scripts/native_simulator/ runner_lib
cd /tmp/native_simulator_build
get-bc -o runner.bca runner.a
${LLVM_PATH}/bin/llvm-link -o runner.bc runner.bca
${LLVM_PATH}/bin/llvm-dis -o runner.ll runner.bc

```
