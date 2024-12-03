import dlist
from pprint import pp
from typing import Annotated

from pyirinterpreter import PyIRModuleInterpreter
from diff import cmp
from proxy import StructProxy, ArraySize

# TODO: fix arrays
# TODO: fix handeling dlists
# TODO: cleaup debug code

from dataclasses import dataclass
from typing import Type, Any, Literal, Union

# @dataclass
# class MyClass:
#     a: int = 42
#     b: str = ""
# 
# Members = Union[str, Literal['a', 'b']]
# 
# def get_type_of(cls: Type[Any], member: Members) -> Any:
#     if hasattr(cls, member):
#         member_value = getattr(cls, member)
#         return type(member_value)
#     else:
#         raise AttributeError(f"'{member}' does not exist in '{cls.__name__}'")
# 
# print(get_type_of(MyClass, 'a'))

# def add(a, b):
#     a.next.cache = b
#     b.prev.cache = a

def add(proxy, ta, tb, field):
    getattr(ta, field).next.cache = proxy.cache_offset(tb, [field])
    getattr(tb, field).prev.cache = proxy.cache_offset(ta, [field])

def init_t(proxy, t):
    t[0].list.prev.ptr = 0
    t[2].list.next.ptr = 0
    # t[0].list.prev.cache = proxy.cache_offset(t[0], ['list'])
    # t[2].list.next.cache = proxy.cache_offset(t[2], ['list'])

    add(proxy, t[0], t[1], 'list')
    add(proxy, t[1], t[2], 'list')
    
def main():
    irm = PyIRModuleInterpreter()
    proxy = StructProxy(irm)

    paths = ["dlist.ll"]
    functionName = b"print"

    if not irm.parseIRFiles(paths):
        raise FileNotFoundError(f"Failed to parse LLVM-IR file {str(paths)}")
    
    irm.createInterpreter()
    
    #t = create_t()
    #pp(t)
    
    t = proxy.Global(Annotated[list[dlist.k_thread], ArraySize(3)], 't')
    # t = proxy.Global(dlist.k_thread, 't')
    # t = [dlist.k_thread(), dlist.k_thread(), dlist.k_thread()]
    
    init_t(proxy, t)
    
    proxy.put(t[0])
    proxy.put(t[1])
    proxy.put(t[2])
    # or enable deep: proxy.put(t[0], true)
    
    pp(t)
    
    # if not irm.runFunction(functionName, [t_list.ptr], args_type_hints=['PTR']):
    if not irm.runFunction(functionName, [t[0].ptr + proxy.offset_of(t[0], ['list'])], args_type_hints=['PTR']):
        raise RuntimeError(f"Failed to run LLVM Interpreter for function {str(functionName)}")
    
    ret = irm.getExitValue()
    pp(ret['IntVal']['S'])
    
    raise SystemExit


if __name__ == '__main__':
    main()

