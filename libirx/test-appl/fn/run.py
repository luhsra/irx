import fn
from pprint import pp
from typing import Annotated
from pyirinterpreter import PyIRModuleInterpreter
from diff import cmp
from proxy import StructProxy, ArraySize, PTR
from dataclasses import dataclass

def main():
    irm = PyIRModuleInterpreter()
    proxy = StructProxy(irm)
    
    if not irm.parseIRFiles(["fn.ll"]):
        raise FileNotFoundError(f"Failed to parse LLVM-IR file")
    
    irm.createInterpreter()
    
    t = proxy.Global(fn.timeout, 't')
    b = fn.big()
    proxy.put(b)
    proxy.get(b)

    pp(t)
    
    if not irm.runFunction(b"main", [], args_type_hints=[]):
        raise RuntimeError(f"Failed to run LLVM Interpreter for function")
    
    proxy.get(t)
    
    print_fn = irm.find_function_named('print')
    pp(hex(print_fn))
    
    pp(t)
    
    raise SystemExit


if __name__ == '__main__':
    main()
