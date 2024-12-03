from pyirinterpreter import PyIRModuleInterpreter
from typing import Annotated
import struct
import pprint
import inspect
from test_data import thread, _thread_base, _cpu, _dnote

from diff import cmp

def main():
    irm = PyIRModuleInterpreter()

    paths = ["libirx/test.ll", "libirx/test-extern.ll"]
    functionName = b"call_thread_join"

    if not irm.parseIRFiles(paths):
        raise FileNotFoundError(f"Failed to parse LLVM-IR file {str(paths)}")

    # irm.set_global()

    t1 = thread(
        base=_thread_base(
            cpus1=[_cpu(n=1)],
            cpus2=[_cpu(n=2), _cpu(n=3)],
            n=[1, 2, 3, 4],
            pended_on=0xffffffff,
            join_waiters=_dnote(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff),
            id=43
        ),
        id=42,
        running=True,
        cpu=1,
        name="t4",
        ptr=0xffffffff
    )
    t2 = thread(base=_thread_base(cpus1=[_cpu(n=9999)], cpus2=[_cpu(n=0), _cpu(n=0)], n=[0, 0, 0, 0], pended_on=0, join_waiters=_dnote(0, 0, 0, 0), id=0), id=0, running=False, cpu=0, name="", ptr=0)

    thread_ptr = irm.structAlloc("struct.thread")
    print("thread_ptr: ", thread_ptr)

    # value = struct.pack('I', 0xFFFFFFFF)

    # print(hex(id(value)))
    # print(value)

    # irm.irx_struct_set(b"thread", thread_ptr, b"id", value, 4)
    
    t1.ptr = thread_ptr.vvptr # for struct internal pointer we need to use the virtual virtual address space

    irm.set_struct("thread", thread_ptr.vptr, t1)
    
    # info = irm.getStructFieldInfo("thread", ["name"])
    # print(info)
    
    # raise SystemExit

    print("---------")
    print("---------")
    print("---------")

    irm.get_struct("thread", thread_ptr.vptr, t2)
    # irm.get_struct("thread", thread_ptr, t1)

    print("")
    print("")
    print("===================")
    print("")
    print("")
    print(cmp(t2, t2))
    print(cmp(t1, t1))
    print(cmp(t1, t2))

    pprint.pp(t2)
    pprint.pp(thread_ptr)

    # ptr = create_primitive('int32_t') #maybe used for the global or args
    # ptr = create_struct('struct.thread')
    # ptr = create_global('struct.thread')

    # it is ok to use vptr in an argument, unless pointers are compared, than vvptr is required
    args = [thread_ptr.vvptr, 421]

    if not irm.runFunction(functionName, args, args_type_hints=['PTR', 'APInt64']):
        raise RuntimeError(f"Failed to run LLVM Interpreter for function {str(functionName)}")


    raise SystemExit


if __name__ == '__main__':
    main()
