LLVM_PATH ?= /mnt/data/opt/llvm/14

globals.ll: globals.c
	$(LLVM_PATH)/bin/clang -m32 -S -o $@ -emit-llvm -g3 $<
