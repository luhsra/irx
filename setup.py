import os
import subprocess
from setuptools import setup, Extension
from Cython.Build import cythonize

LLVM_PATH = os.getenv('LLVM_PATH')
if LLVM_PATH is None:
    LLVM_PATH = subprocess.check_output("llvm-config-14 --prefix", shell=True).decode('utf-8')

LLVM_CONFIG = f"{LLVM_PATH}/bin/llvm-config"

def llvm_config(args):
    return [
            x.strip('\n') \
                for x in subprocess.check_output(f"{LLVM_CONFIG} {args}", shell=True).decode('utf-8').split(' ') \
                if x != ''
        ]

extensions = [
    Extension(
        "pyirinterpreter",
        include_dirs=[],
        libraries=[],
        extra_compile_args=llvm_config("--cxxflags") + ['-fexceptions', '-g', '-g3', '-ggdb3', '-gdwarf-2'],
        extra_link_args=llvm_config("--link-shared --libs --ldflags --system-libs --libs MCJIT Interpreter ExecutionEngine IRReader"),
        sources=["pyirinterpreter.pyx",
                 "../irinterpreter2-debug.cc",
                 "../irinterpreter2-struct.cc",
                 "../irinterpreter2-global.cc",
                 "../irinterpreter2.cc",
                ],
        language="c++",
    )
]

setup(
    name="pyirinterpreter",
    ext_modules=cythonize(extensions, gdb_debug=True),
)
