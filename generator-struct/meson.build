llvm_modules = ['bitwriter', 'core', 'all-targets', 'x86', 'x86info', 'mcjit']
llvm_version = ['>=14', '<16']
llvm_dep = dependency('llvm', method: 'config-tool', version: llvm_version, include_type: 'system', modules: llvm_modules)

source_files = files(
  'generator-struct.cc',
  '../libirx/irinterpreter2-debug.cc',
  '../libirx/irinterpreter2-logging.cc',
  '../libirx/irinterpreter2-struct.cc',
  '../libirx/irinterpreter2-mmap.cc',
)

generator_struct = executable('generator-struct', source_files, dependencies: [llvm_dep])
