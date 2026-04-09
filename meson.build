project('pyirinterpreter', ['cpp', 'cython'],
    meson_version: '>=1.10.0', version: 'SRA\'26',
    default_options: ['warning_level=3',
                      'cpp_std=c++17',
                      'buildtype=debugoptimized'])
py = import('python').find_installation('python3')
cython = find_program('cython', version: '>=3.0.0')
cython_flags = ['-Wextra', '--cplus', '-3', '--cimport-from-pyx', '--capi-reexport-cincludes', '--gdb', '--embed-positions', '--verbose']
llvm_modules = ['mcjit', 'interpreter', 'executionengine', 'irreader', 'linker']
llvm_version = ['>=9', '<16']
llvm_dep = dependency('llvm', method: 'config-tool', version: llvm_version, include_type: 'system', modules: llvm_modules)
ln = find_program('ln')

# source_dir = meson.current_source_dir()
# setup_py_target = custom_target(
#   'setup_py_target',
#   output: 'setup_output',
#   command: [py, source_dir + '/setup.py', 'build_ext'],
#   build_by_default: true
# )

add_project_arguments('-Wno-unused-variable' , language : 'cpp')
add_project_arguments('-Wno-unused-parameter' , language : 'cpp')

project_name = 'pyirinterpreter-py' + py.language_version()
pyirinterpreter_source_files = files(
  'libirx/irinterpreter2-debug.cc',
  'libirx/irinterpreter2-struct.cc',
  'libirx/irinterpreter2-global.cc',
  'libirx/irinterpreter2-logging.cc',
  'libirx/irinterpreter2-interpreter.cc',
  'libirx/irinterpreter2-mmap.cc',
  'libirx/irinterpreter2-sections.cc',
)

cython_sources = ['pyirinterpreter.pyx']
headers = ['pyirinterpreter.pxd']

irx_dir = meson.current_build_dir()
source_dir = meson.current_source_dir()

pyirinterpreter = py.extension_module('pyirinterpreter',
    cython_sources + pyirinterpreter_source_files,
    install: true,
    gnu_symbol_visibility: 'default',
    dependencies: [py.dependency(), llvm_dep],
    override_options : ['cython_language=cpp'],
    cpp_args : [
      '-fexceptions',
      '-O2',
#       '-g', '-g3', '-ggdb3', '-gdwarf-2'
    ],
    cython_args : cython_flags,
)

py_files = [
  'pyirinterpreter.pyx',
  'diff.py',
  'proxy.py',
  'zephyr.py',
]
py_targets = []

foreach file : py_files
  py_targets += custom_target(
    file,
    input: source_dir / file,
    output: file,
    command: [ln, '-sfn', '@INPUT@', '@OUTPUT@'],
    build_by_default: true
  )
endforeach

irx_tgt = py_targets + [pyirinterpreter]

nop = find_program('true')
run_target('irx', command: nop, depends: irx_tgt)

if get_option('build_irx-tests')
  subdir('test')
  subdir('libirx/tests')
endif
subdir('extract-blob')
subdir('generator-struct')
subdir('native_runner')
