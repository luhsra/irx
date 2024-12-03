# README
```sh
export PYTHONPATH=../../../:../../../lib64/python3.11/site-packages

gcc -o fn -Wall fn.cc
${LLVM_PATH}/bin/clang -m32 -S -o fn.ll -emit-llvm -g3 fn.cc
../../../builddir/generator-struct/generator-struct fn.ll -o fn.py
python3 run.py

build() {
    meson compile -C ../../../builddir
    meson install -C ../../../builddir
}

python3 -m cProfile -o /tmp/irx.profile run.py
python3 -m snakeviz -s /tmp/irx.profile

valgrind --tool=callgrind python3 run.py

```
