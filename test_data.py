from dataclasses import dataclass
from typing import Annotated
import struct

"""Annotation"""
@dataclass
class ArraySize:
    value: int

"""Annotation"""
@dataclass
class ctype:
    kind: str

@dataclass
class _dnote:
    head: int
    next: int
    tail: int
    prev: int

@dataclass
class _cpu:
    n: int

@dataclass
class _thread_base:
    cpus1: Annotated[list[_cpu], ArraySize(1)]
    cpus2: Annotated[list[_cpu], ArraySize(2)]
    n: Annotated[list[int], ArraySize(8)]
    pended_on: int
    join_waiters: _dnote
    id: int

@dataclass
class thread:
    base: _thread_base
    id: int
    running: bool
    cpu: int
    name: str
    ptr: int
