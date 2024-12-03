from dataclasses import dataclass
from typing import Annotated

"""Annotation"""
@dataclass
class ArraySize:
    value: int

@dataclass
class _cpu:
    n: int

@dataclass
class thread:
    cpus1: Annotated[list[_cpu], ArraySize(1)]
    cpus2: Annotated[list[_cpu], ArraySize(2)]
    n: Annotated[list[int], ArraySize(8)]
    name: str

