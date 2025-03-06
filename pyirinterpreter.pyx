# distutils: language=c++
# cython: language_level=3

include "pyirinterpreter.pxd"


from libcpp cimport bool
from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp.vector cimport list as stdList
from libc cimport stdint
from libc.stdint cimport uintptr_t
from cpython.ref cimport PyObject
from cpython.mem cimport PyMem_Malloc, PyMem_Realloc, PyMem_Free
from cpython.bytes cimport PyBytes_FromStringAndSize, PyBytes_FromString
from cpython.string cimport PyString_FromString
from cpython.long cimport PyLong_FromLong, PyLong_FromLongLong
from cython.operator cimport dereference as deref

from dataclasses import dataclass, fields, is_dataclass
import struct

import logging
# logging.basicConfig(level=logging.NOTSET)
# logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("cython:irinterpreter.pyx")
logger.setLevel(logging.INFO)

ctypedef void *PTR

class PyIRXAlloc:
    def __init__(self, vvptr, vptr, size):
        self.vvptr = vvptr
        self.vptr = vptr
        self.size = size
    
    def __repr__(self):
        return f"[vv@{hex(self.vvptr)} v@{hex(self.vptr)} size:{self.size}]"

cdef class PyIRModuleInterpreter:
    cdef IRModuleInterpreter *irx

    def __cinit__(self):
        self.irx = new IRModuleInterpreter()

    def __dealloc__(self):
        del self.irx

    def parseIRFiles(self, list[string] InputFilenames, name = b"CompositeModule"):
        cdef vector[string] vFiles
        for f in InputFilenames:
            vFiles.push_back(f.encode("utf-8"))
        return self.irx.parseIRFiles(vFiles, name)
    
    def createInterpreter(self) -> bool:
        return self.irx.createInterpreter()

    def runFunction(self, string FunctionName, args, args_type_hints):
        logger.debug(f"Run {FunctionName} len(args): {len(args)}")
        cdef vector[llvmGenericValue] Args
        Args.resize(len(args))

        # Args[0] = llvmGenericValue(<void*><stdint.uintptr_t>args[0])
        # Args[1].IntVal = APInt(64U, <unsigned long>args[1], <bool>False)
        for i in range(len(args)):
            logger.debug(type(args[i]).__name__)
            Args[i] = PyIRModuleInterpreter.convert2GenericValue(args[i], args_type_hints[i])

        return self.irx.runFunction(FunctionName, Args)

    def structAlloc(self, str structName):
        alloc = self.irx.structAlloc(<string>structName.encode())
        return PyIRXAlloc(alloc.vvptr, <stdint.uintptr_t> alloc.vptr, alloc.size)

    #def structInfo(self, str name):
    #    return self.irx.structInfo(name)

    def offsetInBitsFromPath(self, structName, list path) -> int:
        cdef vector[string] vpath
        for e in path:
            vpath.push_back(e.encode())
        return self.irx.offsetInBitsFromPath(structName.encode(), vpath);
    
    def offset_of(self, cls, path) -> int:
        nbits = self.offsetInBitsFromPath(cls.__name__, path)
        if nbits & 0x07 != 0:
            raise RuntimeError(f"Got an offset {nbits} bits and is not dividable by 8")
        return nbits // 8
    
    def get_alloc_size(self, structName) -> int:
        return self.irx.irx_struct_get_size(structName.encode())
    
    def irx_struct_set(self, str name, stdint.uintptr_t m, list path, bytes value):
        # cdef unsigned char[:] c_value = value[:len(value)]
        cdef unsigned char* c_value = value
        bytesName = name.encode()
        #bytesField = field.encode()

        cdef vector[string] vpath
        for e in path:
            vpath.push_back(e)

        self.irx.irx_struct_set(<const char*>bytesName, <void*> m, <vector[string]> vpath, <void*> c_value, len(value))
    
    def get_ptr_struct_type(self):
        ptr_type = 'P' # native pointer type
        if self.getPointerSize() == 32:
            ptr_type = 'I'
        elif self.getPointerSize() == 64:
            ptr_type = 'Q'
        else:
            logger.warning("could not detect pointer size, using native ptr size")
        return ptr_type
    
    def set_struct_primitive(self, str name, stdint.uintptr_t thread_ptr, data, path=[], depth=0):
        k = path[-1]
        sp = ' ' * depth # space prefix
        # info = self.irx.getStructFieldInfo(name.encode(), path)
        info = self.irx.getStructFieldInfo2(name.encode(), path)
        logger.debug("%spyx path: %s basicType:%s size_in_bytes:%s gep:%s" % (sp, str(path), info.basicType, info.size_in_bytes, info.gep))
        
        path_str = '.'.join(map(lambda x : x.decode('utf8'), path))
        
        bt = info.basicType.decode('utf8')
        
        if bt in ["unsigned int"] and isinstance(data, int): # make sure both python and LLVM-IR is the compatible
            value = struct.pack('I', data)
            self.irx_struct_set(name, thread_ptr, path, value)
        elif bt in ["int"]:
            value = struct.pack('i', data)
            self.irx_struct_set(name, thread_ptr, path, value)
        elif bt in ["bool"]: #   and isinstance(data, bool)
            value = struct.pack('?', data)
            self.irx_struct_set(name, thread_ptr, path, value)
        elif bt in ["PTR"]:
            #if type(obj).__name__ == 'PTR':
            #    data = data.ptr
            if data is not None:
                value = struct.pack(self.get_ptr_struct_type(), data)
                self.irx_struct_set(name, thread_ptr, path, value)
        elif bt in ["char"]: # will also handle strings (char[])
            logger.debug(f"{sp}pyx handling type: {info.basicType} for field `{k}` data: {data}")
            # value = struct.pack('b', data)
            value = data.ljust(info.size_in_bytes, '\x00').encode('ascii')
            self.irx_struct_set(name, thread_ptr, path, value)
        elif bt in ["unsigned char"]:
            value = struct.pack('B', data)
            self.irx_struct_set(name, thread_ptr, path, value)
        elif bt in ["unsigned short"]:
            value = struct.pack('H', data)
            logger.debug("irx_struct_set: %s %s %d", bt, path, int.from_bytes(value, "little", signed=False))
            self.irx_struct_set(name, thread_ptr, path, value)
        elif bt in ["short"]:
            value = struct.pack('h', data)
            self.irx_struct_set(name, thread_ptr, path, value)
        elif bt in ["unsigned long long"]:
            value = struct.pack('Q', data)
            self.irx_struct_set(name, thread_ptr, path, value)
        elif bt in ["long long"]:
            value = struct.pack('q', data)
            self.irx_struct_set(name, thread_ptr, path, value)
        # elif bt.startswith('array.'):
            # logger.debug(f"{sp}pyx warning: unhandled array type: {info.basicType} for field `{k}`")
            # value = struct.pack('Q', data)
            # self.irx_struct_set(name, thread_ptr, path, value)
        elif bt in ["unknown"]:
            logger.warning(f"{sp}pyx warning: unhandled type: {path_str} {info.basicType} for field `{k}`")
        else:
            raise RuntimeError(f"Convert to C-mem not implemented for field '{k}' type: {bt}")
    
    # this could be replace with ExecutionEngine::InitializeMemory(Constant*)
    # cython would just create the Constant and llvm would write it to memory
    # pro: can be dumped(better debug)
    def set_struct(self, str name, stdint.uintptr_t thread_ptr, obj, path=[], depth=0):
        logger.debug("%s| %s, %s, %s", ' ' * depth, "pyx set_struct", type(obj).__name__, str(isinstance(obj, list)))
        
        if type(obj) in [int, bytes, str] or type(obj).__name__ == 'bool':
            self.set_struct_primitive(name, thread_ptr, obj, path, depth+1)
        elif obj is None:
            pass
        elif type(obj).__name__ == 'PTR':
            self.set_struct_primitive(name, thread_ptr, obj.ptr, path, depth)
        elif type(obj) in [list]:
            logger.debug(' ' * depth + "found: [list]")
            for k, v in enumerate(obj):
                self.set_struct(name, thread_ptr, v, path + [str(k).encode()], depth+1)
        elif type(obj) in [dict]:
            logger.debug(' ' * depth + "found: [dict]")
            for k in list(vars(obj).keys()):
                data = getattr(obj, k.name)
                self.set_struct(name, thread_ptr, data, path + [str(k).encode()], depth+1)
        elif is_dataclass(obj):
            logger.debug(' ' * depth + "found: [is_dataclass]")
            for k in fields(obj):
                if k.name.startswith("__"):
                    continue
                data = getattr(obj, k.name)
                self.set_struct(name, thread_ptr, data, path + [k.name.encode()], depth+1)
        else:
            raise RuntimeError(f"Convert to C-mem not implemented for field '{path}'")
    
    def get_struct_primitive(self, str name, stdint.uintptr_t thread_ptr, data, path=[], depth=0):
        # TODO remove data from func arg list: this arg should not be required
        k = path[-1]
        
        info = self.irx.getStructFieldInfo2(name.encode(), path)
        logger.debug("getStructFieldInfo returned to python")
        len = info.size_in_bytes
        logger.debug(f"pyx calling: PyMem_Malloc {len}")

        if len <= 0:
            logger.warning(f"Invalid len value: {len} <= 0")
            return None
            # raise SystemError(f"Invalid len value: {len} <= 0")

        mem = <char*> PyMem_Malloc(len * sizeof(char))
        if not mem:
            raise MemoryError()

        # cdef char *buf = (char *) malloc(BUFSIZ)
        # if (buf == NULL)
        #     return PyErr_NoMemory();

        logger.debug("pyx calling: irx_struct_get")
        self.irx.irx_struct_get(name.encode(), <void*> thread_ptr, path, <void*> mem, len)
        logger.debug("pyx returned: irx_struct_get")

        pydata = None;

        basicType = info.basicType.decode('utf8')

        pathstr = '.'.join(map(lambda x: x.decode(), path))

        logger.debug(f"  pyx 1 {basicType} for field `{pathstr}`")
        if basicType in ["PTR"]: # TODO replace with info from llvm, ...
            
            if self.getPointerSize() == 32:
                pydata = PyLong_FromLong(deref(<unsigned long*> mem) & 0xFFFFFFFF)
            else:
                pydata = PyLong_FromLongLong(deref(<unsigned long long*> mem))
            
            # pydata = struct.unpack(self.get_ptr_struct_type(), pydata)
        elif basicType in ["char"] and len > 1:
            pydata = PyBytes_FromStringAndSize(mem, len)
            try:
                pydata = str(pydata, 'UTF8').rstrip('\x00') # ATTENTION: could alter data. TODO: need better strategy to decide
            except:
                pass
            logger.debug(f"  pyx 2 {basicType} for field `{pathstr}` = pydata rstrip {pydata}")
        elif isinstance(data, int) and basicType in ["unsigned int", "int", "unsigned long", "long"]:
            pydata = PyLong_FromLong(deref(<int*> mem))
        elif isinstance(data, int) and basicType in ["char"]:
            pydata = PyLong_FromLong(deref(<char*> mem) & 0xFF)
        elif isinstance(data, int) and basicType in ["unsigned char"]:
            pydata = PyLong_FromLong(deref(<unsigned char*> mem) & 0xFF)
        elif isinstance(data, int) and basicType in ["unsigned short"]:
            pydata = PyLong_FromLong(deref(<unsigned short*> mem) & 0xFFFF)
        elif isinstance(data, int) and basicType in ["short"]:
            pydata = PyLong_FromLong(deref(<short*> mem) & 0xFFFF)
        elif basicType in ["bool"]:
            boolbyte = deref(<unsigned char*> mem) & 0xFF
            pydata = PyLong_FromLong(boolbyte) != 0
        elif isinstance(data, int) and basicType in ["unsigned long long", "long long"]:
            pydata = PyLong_FromLong(deref(<long*> mem))
        elif basicType in ["unknown"]:
            logger.warning(f"pyx warning: unhandled type: {info.basicType} for field `{pathstr}`")
        else:
            raise SystemError(f"Convert to C-mem not implemented for field '{pathstr}'. type: {basicType}")

        logger.debug(f"  pyx 3 {basicType} for field `{pathstr}` = pydata {pydata}")

        return pydata

    def get_struct(self, str name, stdint.uintptr_t thread_ptr, obj, path=[], depth=0): # TODO: add parent (parent.obj), alernative: use the return value
        logger.debug("%s| %s, %s, %s" % (' ' * depth, "pyx get_struct", type(obj).__name__, str(isinstance(obj, list))))

        if type(obj) in [int, bytes, str] or type(obj).__name__ == 'bool':
            pydata = self.get_struct_primitive(name, thread_ptr, obj, path, depth+1)
            return pydata
            #setattr(obj, k, pydata)
            #obj = pydata
        elif type(obj).__name__ == 'PTR':
            obj.ptr = self.get_struct_primitive(name, thread_ptr, obj, path, depth+1)
            return obj
        elif obj is None:
            pass
        elif type(obj) in [list]:
            logger.debug(' ' * depth + "found: [list]")
            for k, v in enumerate(obj):
                pydata = self.get_struct(name, thread_ptr, v, path + [str(k).encode()], depth+1)
                obj[k] = pydata # probably inefficient
        elif type(obj) in [dict]:
            logger.debug(' ' * depth + "found: [dict]")
            for k in list(vars(obj).keys()):
                data = getattr(obj, k.name)
                pydata = self.get_struct(name, thread_ptr, data, path + [str(k).encode()], depth+1)
                setattr(obj, k, pydata) # probably inefficient
        elif is_dataclass(obj):
            logger.debug(' ' * depth + "found: [is_dataclass]")
            for k in fields(obj):
                if k.name.startswith("__"):
                    continue
                data = getattr(obj, k.name)
                pydata = self.get_struct(name, thread_ptr, data, path + [k.name.encode()], depth+1)
                setattr(obj, k.name, pydata) # probably inefficient
        else:
            raise RuntimeError(f"Convert to C-mem not implemented for field '{path}'")
        return obj


    def getStructFieldInfo(self, str name, list _needle) -> BasicTypeInfo:
        logger.debug("getStructFieldInfo name:", name, _needle)
        cdef vector[string] needle
        for e in _needle:
            needle.push_back(e.encode())
        
        return self.irx.getStructFieldInfo2(name.encode(), <vector[string]> needle)

    def irx_global_set(self, str name, bytes value):
        # todo reuse set_struct_primitive
        cdef unsigned char* c_value = value
        bytesName = name.encode()
        self.irx.irx_set_global(<const char*>bytesName, <void*> c_value, len(value))
    
    def create_global(self, str name, str typeName) -> bool:
        return self.irx.irx_create_global(name.encode(), typeName.encode())
    
    def setInitializer(self, str name, str typeName) -> bool:
        return self.irx.setInitializer(name.encode(), typeName.encode())
    
    def hasExternal(self, str name) -> bool:
        return self.irx.hasExternal(name.encode())
    
    def irx_global_set_ptr(self, str name, uintptr_t data):
        # todo reuse set_struct_primitive
        bytesName = name.encode()
        value = struct.pack(self.get_ptr_struct_type(), data)
        self.irx.irx_set_global(<const char*>bytesName, <void*> value, self.getPointerSizeByte())
    
    def irx_get_global_addr(self, str name):
        bytesName = name.encode()
        return <stdint.uintptr_t> self.irx.irx_get_global_addr(<const char*>bytesName)
    
    def irx_get_global_var(self, str name):
        bytesName = name.encode()
        genericValue = self.irx.irx_get_global_var(<const char*>bytesName)
        return PyIRModuleInterpreter._convertGenericValueToPython(genericValue)
    
    # todo add actual get , reuse get_struct_primitive
    
    def vvptr2vptr(self, uintptr_t vvptr) -> stdint.uintptr_t:
        return <stdint.uintptr_t> self.irx.vvptr2vptr(vvptr)
    
    def vptr2vvptr(self, uintptr_t vptr) -> stdint.uintptr_t:
        return self.irx.vptr2vvptr(<void *>vptr)
    
    def getDeclaredFunction(self) -> list[str]:
        functions = list[str]()
        for f in self.irx.getDeclaredFunction():
            functions.append(f.decode("utf-8"))
        return functions
    
    def isFunctionDeclared(self, string name) -> bool:
        self.irx.isFunctionDeclared(name.encode())
    
    def getPointerSize(self):
        return self.irx.getPointerSize()
    
    def getPointerSizeByte(self):
        return self.getPointerSize() / 8
    
    def stop(self, list functions):
        """ Pass a list of function at which the interpreter should stop the execution """
        cdef vector[string] v
        for f in functions:
            v.push_back(f.encode())
        return self.irx.stop(v)
    
    def skip(self, list functions):
        """ Pass a list of function at which the interpreter should skip the instruction """
        cdef vector[string] v
        for f in functions:
            v.push_back(f.encode())
        return self.irx.skip(v)
    
    def set_print_llvmir(self, printit = True):
        return self.irx.setPrintLLVMIR(printit)
    
    
    @staticmethod
    cdef dict _convertGenericValueToPython(llvmGenericValue a):
        return {
            "IntVal": {
                    "Z": a.IntVal.getZExtValue(),
                    "S": a.IntVal.getSExtValue(),
                },
            "DoubleVal": a.DoubleVal,
            "FloatVal": a.FloatVal,
            "PointerVal": <stdint.uintptr_t> a.PointerVal,
        }
    
    def getCurrentCallArgs(self):
        args = []
        v = self.irx.getCurrentCallArgs()
        for a in v:
            args.append(PyIRModuleInterpreter._convertGenericValueToPython(a))
        return args
    
    def resume(self):
        return self.irx.resume()
    
    def getExitValue(self):
        return PyIRModuleInterpreter._convertGenericValueToPython(self.irx.getExitValue())

    def getCallStack(self):
        args = []
        v = self.irx.getCallStack()
        for a in v:
            args.append(a.decode('utf8'))
        return args
    
    # if the interpreter was stoped on a call this method will return the name of the function to be called
    def getCurrentCallFunctionName(self):
        return self.irx.getCurrentCallFunctionName().decode('utf8')
    
    def isRunnable(self) -> bool:
        return self.irx.isRunnable()
    
    @staticmethod
    cdef llvmGenericValue convert2GenericValue(arg, type_hint):
        cdef llvmGenericValue Result = llvmGenericValue()
        if type(arg).__name__ == 'int':
            if type_hint == 'APInt32':
                Result.IntVal = APInt(32U, <unsigned long>arg, <bool>False)
            elif type_hint == 'APInt64':
                Result.IntVal = APInt(64U, <unsigned long>arg, <bool>False)
            elif type_hint == 'PTR':
                Result = llvmGenericValue(<void*><stdint.uintptr_t>arg)
            else:
                raise TypeError("No args hint unknown")
        else:
            raise TypeError(f"Unknown arg type {type(arg).__name__}")
        
        return Result
    
    def putReturnValue(self, value, type_hint):
        cdef llvmGenericValue Arg = PyIRModuleInterpreter.convert2GenericValue(value, type_hint)
        return self.irx.putReturnValue(Arg)

    def find_function_by_vvptr(self, int vvptr) -> str:
        return self.irx.findFunctionByVVPtr(vvptr).decode('utf8')
    
    def find_function_named(self, str FunctionName) -> int:
        bytesName = FunctionName.encode()
        return self.irx.FindFunctionNamed(<const char*>bytesName)
    
    def get_global_value_at_address(self, int vvptr) -> str:
        return self.irx.getGlobalValueAtAddress(vvptr).decode('utf8')
    
    
    def reset(self):
        """Only reset the memory address allocator but keep module"""
        self.irx.resetExtraAllocations()
