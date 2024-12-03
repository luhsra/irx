from __future__ import annotations
from dataclasses import dataclass, field
from typing import Annotated, Optional, Union, TypedDict, get_args
import ctypes
import pprint

from dataclasses import dataclass, fields, is_dataclass

from diff import diff, cmp

import logging
# logging.basicConfig(level=logging.NOTSET)
# logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("zephyr.py")

@dataclass
class VOID:
    pass

# Need to fix: multiple obj could have the same address
# For example the struct itself and the first element in that struct
# currently we will just avoid this case

class ObjectMap(TypedDict):
    # map all object with there id to the ptr that was fetched
    id: id
    ptr: int

class ReverseObjectMap(TypedDict):
    ptr: int
    id: id

UInt8  = Annotated[int, ctypes.c_uint8]
UInt16 = Annotated[int, ctypes.c_uint16]
UInt32 = Annotated[int, ctypes.c_uint32]
UInt64 = Annotated[int, ctypes.c_uint64]
Int8   = Annotated[int, ctypes.c_int8]
Int16  = Annotated[int, ctypes.c_int16]
Int32  = Annotated[int, ctypes.c_int32]
Int64  = Annotated[int, ctypes.c_int64]

@dataclass
class ObjectOffset:
    object: object
    offset: int

@dataclass
class ArraySize:
    value: int

@dataclass
class ctype:
    kind: str

@dataclass
class PTR:
    cls: type
    ptr: int = None
    
    cache: ObjectOffset = None # pointer to object. to detect this case and automaticallaly use a pointer for the object instead of a struct. maybe also need to manage a map of python id(object) to vvptr addresses to find the same objects and reuse them (or safe the vvptr as an annotation into the object itself.
    # only load structs from pointer if accessed
    
#     def __get__(self, instance, owner):
#         return self.cache
#     
#     def __set__(self, instance, value):
#         self.cache = value
    
    def __repr__(self):
        ptr = self.ptr if self.ptr is not None else 0
        return f"PTR({hex(ptr)} {self.cls.__name__})"



class StructProxy:
    def __init__(self, irx):
        self.objectMap = ObjectMap()
        self.reverseObjectMap = ReverseObjectMap()
        self.irx = irx
    
    def get(self, obj, ptr: PTR = None):
        assert(ptr is None or isinstance(obj, ptr.cls))
        assert(obj.__ptr__.ptr is None or ptr is None)
        
        if obj.ptr is None and ptr is not None:
            obj.ptr = ptr.ptr
        
        assert(obj.ptr is not None and obj.ptr != 0)
        self.irx.get_struct(obj.__ptr__.cls.__name__, obj.ptr, obj)
        # should also cache the cls, or else address with same address
        # but different type will return the wrong obj
        self.objectMap[obj.ptr] = obj
        self.reverseObjectMap[id(obj)] = obj.ptr
    
    def deref_array(self, ptr: PTR):
        if hasattr(ptr.cls, '__metadata__'):
            for annotation in ptr.cls.__metadata__:
                if isinstance(annotation, ArraySize):
                    type_args = get_args(ptr.cls.__origin__)
                    ptr.cls = type_args[0]
                    size = self.irx.get_alloc_size('struct.' + ptr.cls.__name__)
                    arr = []
                    for i in range(annotation.value):
                        arr.append(self.deref(ptr))
                        ptr.ptr += size
                    return arr
        
        return self.deref(ptr)
    
    def deref_no_cache(self, ptr: PTR):
        obj = ptr.cls()
        obj.ptr = ptr.ptr
        self.irx.get_struct(obj.__ptr__.cls.__name__, obj.ptr, obj)
        return obj
    
    def deref(self, ptr: PTR):
        if ptr.ptr not in self.objectMap:
            obj = ptr.cls()
            self.get(obj, ptr)
        
        return self.objectMap[ptr.ptr]
    
    def put_cache(self, obj):
        if is_dataclass(obj):
            for m in fields(obj):
                if not m.name.startswith("__"):
                    ptr = getattr(obj, m.name)
                    if m.type == 'PTR':
                        if ptr.cache is not None:
                            new = ptr.cache.object
                            offset = ptr.cache.offset
                            ptr.cache = None
                            if id(new) not in self.reverseObjectMap:
                                self.put(new)
                            ptr.ptr = new.ptr + offset
                            
                    elif is_dataclass(ptr):
                        self.put_cache(ptr)
        
    def put(self, obj, deep=False):
        if obj.ptr is None:
            ptr = self.irx.structAlloc("struct." + obj.__ptr__.cls.__name__)
            obj.ptr = ptr.vvptr
            
            self.objectMap[obj.ptr] = obj
            self.reverseObjectMap[id(obj)] = obj.ptr
        
        self.put_cache(obj)
        
        if deep:
            uploaded = [obj.ptr]
            for m in fields(obj):
                if m.type == 'PTR' and not m.name.startswith("__"):
                    ptr = getattr(obj, m.name)
                    if ptr.ptr in self.objectMap:
                        if ptr.ptr not in uploaded:
                            uploaded += [ptr.ptr]
                            self.put(self.deref(ptr))
        
        obj2 = obj.__ptr__.cls()
        self.irx.set_struct(obj.__ptr__.cls.__name__, obj.ptr, obj)
        self.irx.get_struct(obj.__ptr__.cls.__name__, obj.ptr, obj2)
        
        mismatches = diff(obj, obj2)
        [logger.error(f"Missmatch: {m}") for m in mismatches]
        assert(len(mismatches) == 0)
    
    def Global(self, cls, global_name: str):
        vvptr = self.irx.irx_get_global_addr(global_name)
        if vvptr == 0:
            return None
        return self.deref_array(PTR(cls, vvptr));

    def offset_of(self, obj, path) -> int:
        return self.offset_of_cls(type(obj), path)
    
    def offset_of_cls(self, cls, path) -> int:
        return self.irx.offset_of(cls, path)
    
#     def get_type_from_path(self, path):
#         return ...
#     
#     def offset(self, obj, path) object:
#         """Offset will return the python object embedded in ob at the provided path"""
#         """this would only work for object already 'put' into the LLVM Interpreter"""
#         vvoffset = self.offset_of(obj, path)
#         cls = self.get_type_from_path(type(obj), path)
#         newobj = cls()
#         #newobj.ptr = obj.ptr + vvoffset
#         return newobj
    
    def cache_offset(self, obj, path) -> ObjectOffset:
        return ObjectOffset(obj, self.offset_of(obj, path))
    
    def container_of(self, obj, cls, name, force_new = True):
        return self.container_of_ptr(obj.__ptr__, cls, name, force_new)
    
    def container_of_ptr(self, ptr, cls, name, force_new = True):
        # create PTR and self.deref/self.get could result in the wrong object (from cache), so avoid it here by default.
        # if we know that the pointer will result in a object that we directly created ourself it is valid to use with
        # force_new = False
        newptr = PTR(cls)
        newptr.ptr = ptr.ptr - self.offset_of_cls(cls, [name])
        if force_new:
            new = cls()
            new.__ptr__ = newptr
            self.irx.get_struct(new.__ptr__.cls.__name__, new.ptr, new)
            return new
        else:
            new = self.deref(newptr) # probably got a cached version
            self.get(new, newptr) # avoid the cache and update cached object
            return new
    
    def get_member(self, obj, attrs):
        return self.update_ptr(obj, attrs)
    
    def update_ptr(self, obj, attrs):
        target = obj
        for attr in attrs:
            target = getattr(target, attr)
        target.ptr = obj.ptr + self.offset_of(obj, attrs)
        return target
