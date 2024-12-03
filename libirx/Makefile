LLVM_PROJECT_SRC_PATH ?= /srv/scratch/vit.fendel/llvm-project
LLVM_PATH ?= $(shell llvm-config-14 --prefix)
LLVM_CONFIG := $(LLVM_PATH)/bin/llvm-config

PKGAPPS := python3
LDFLAGS += $(shell pkg-config --libs $(PKGAPPS)) $(shell $(LLVM_CONFIG) --link-shared --libs --ldflags --system-libs --libs MCJIT Interpreter ExecutionEngine IRReader Linker)
# # LDFLAGS += -lclang -lclang-cpp -lclangAnalysis -lclangAST -lclangBasic -lclangCodeGen -lclangEdit -lclangFormat -lclangFrontend \
	# -lclangInterpreter -lclangLex -lclangParse -lclangRewrite
# LDFLAGS += -lclang-cpp
# LDFLAGS += -Wl,-z,origin

CPPFLAGS += $(shell pkg-config --cflags $(PKGAPPS)) -DINTERPRETER_ENABLE_INT3
CCOMMONFLAGS += -pipe -g -gdwarf-2 -Wall -Wextra -pedantic -Wno-unused-parameter
CFLAGS += $(CCOMMONFLAGS) $(CPPFLAGS) -std=c17 $(shell $(LLVM_CONFIG) --cflags)
CXXFLAGS += $(CCOMMONFLAGS) $(CPPFLAGS) -std=c++14 $(shell $(LLVM_CONFIG) --cxxflags)
CXXFLAGS += -fexceptions

TARGET := irinterpreter2
OBJ := $(TARGET).o irinterpreter2-struct.o irinterpreter2-debug.o irinterpreter2-global.o irinterpreter2-logging.o irinterpreter2-interpreter.o irinterpreter2-mmap.o irinterpreter2-sections.o

# LLVM_IR_FILES := zephyr.elf.ll linker-globals.ll test.ll test-extern.ll
LLVM_IR_FILES := test-appl/simple/test.ll test-appl/simple/test-extern.ll
# FUNC := z_impl_k_thread_join
FUNC := call_thread_join

$(TARGET): $(OBJ)
	$(CXX) -o $@ $(OBJ) $(LDFLAGS)

debug: CPPFLAGS += -DDEBUG
debug: CXXFLAGS += -g3 -ggdb3
debug: CFLAGS += -g3 -ggdb3
debug: $(TARGET)

release: CPPFLAGS += -DRELEASE
release: CXXFLAGS += -O2 # -march=x86-64-v3
release: CFLAGS += -O2 # -march=x86-64-v3
release: LDFLAGS += -s
release: $(TARGET)

.PHONY: clean
clean:
	rm -rfv $(TARGET) $(OBJ) test-appl/simple/test.{elf,ll,s}

CLANG_ARGS := -m32
%.ll: %.cc
	$(LLVM_PATH)/bin/clang++ $(CLANG_ARGS) -o $@ -S -emit-llvm -g3 $<

.PHONY: run
run: $(TARGET) $(LLVM_IR_FILES)
# 	LD_LIBRARY_PATH=$(shell $(LLVM_CONFIG) --libdir) ./$(TARGET) test.cc print_max
	LD_LIBRARY_PATH=$(shell $(LLVM_CONFIG) --libdir) ./$(TARGET) --func $(FUNC) $(LLVM_IR_FILES)

.PHONY: gdb
gdb: $(TARGET) $(LLVM_IR_FILES)
# 	LD_LIBRARY_PATH=$(shell $(LLVM_CONFIG) --libdir) ./$(TARGET) test.cc print_max
	LD_LIBRARY_PATH=$(shell $(LLVM_CONFIG) --libdir) gdb -x .gdbinit -ex 'b IRModuleInterpreter::getPointerSize' -ex run --args ./$(TARGET) --func $(FUNC) $(LLVM_IR_FILES)

# TODO create a version directory llbm-14 llvm-${VERSION}/Interpreter.h
llvm-internal/Interpreter.h: $(LLVM_PROJECT_SRC_PATH)/llvm/lib/ExecutionEngine/Interpreter/Interpreter.h
	@mkdir -p $(shell dirname $@)
	cp -v $< $@
	patch -p0 -i Interpreter.patch

Interpreter.patch: llvm-internal/Interpreter.h
	diff -Naur $(LLVM_PROJECT_SRC_PATH)/llvm/lib/ExecutionEngine/Interpreter/Interpreter.h llvm-internal/Interpreter.h | sed s!$(LLVM_PROJECT_SRC_PATH)/!!g > $@

.PHONY: compile_commands.json
compile_commands.json:
	make clean
	bear -- make
