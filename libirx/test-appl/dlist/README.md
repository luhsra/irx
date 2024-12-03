# README
```sh
export PYTHONPATH=../../../:../../../lib64/python3.11/site-packages

gcc -o dlist -Wall dlist.cc
${LLVM_PATH}/bin/clang -S -o dlist.ll -emit-llvm -g3 dlist.cc
../../../builddir/generator-struct/generator-struct dlist.ll -o dlist.py
python3 run_dlist.py

build() {
    meson compile -C ../../../builddir
    meson install -C ../../../builddir
}

```
