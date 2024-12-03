# import fn
import zephyr
from pprint import pp
from typing import Annotated
from pyirinterpreter import PyIRModuleInterpreter
from diff import cmp, diff
from proxy import StructProxy, ArraySize, PTR
from dataclasses import dataclass

def main():
    irm = PyIRModuleInterpreter()
    proxy = StructProxy(irm)

    if not irm.parseIRFiles([
            "../../../../../parrot/build/subprojects/ara-zephyr-apps/appl/native_sim-static_thread_join.ll",
            "../../../../../parrot/build/subprojects/ara-zephyr-apps/appl/native_sim-static_thread_join-kernel.ll",
            "../../../../../parrot/build/subprojects/ara/subprojects/irx/native_runner/runner.ll"
        ]):
        raise FileNotFoundError(f"Failed to parse LLVM-IR file")
        
    irm.createInterpreter()
    
    # t = proxy.Global(fn.timeout, 't')
    b = zephyr.k_thread()
    proxy.put(b)
    proxy.get(b)
    
    irm.skip(['arch_new_thread'])
    if not irm.runFunction(b"z_init_cpu", [0], args_type_hints=['APInt32']):
        raise RuntimeError(f"Failed to run LLVM Interpreter for function")
    
    diff(b, b)
    

if __name__ == '__main__':
    main()
