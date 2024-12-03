set debuginfod enabled off
set confirm off
set print pretty on
set auto-solib-add on

set print elements 0

python
import sys
sys.path.insert(0, '/usr/share/gcc-14/python')
from libstdcxx.v6.printers import register_libstdcxx_printers
try:
  register_libstdcxx_printers(None)
except:
    pass
end

add-symbol-file lib64/python3.11/site-packages/pyirinterpreter.cpython-311-x86_64-linux-gnu.so
# add-symbol-file /mnt/data/opt/llvm/14/lib/libLLVMExecutionEngine.so

set breakpoint pending on

# b process_composite_struct
# b llvm::ExecutionEngine::emitGlobals


# b MMapAllocator::~MMapAllocator
# b IRXAllocator::~IRXAllocator
