# project('extract-blob', ['cpp'],
#     version: '0.0',
#     default_options: ['warning_level=3',
#                       'cpp_std=c++17',
#                       'buildtype=debugoptimized'])

llvm_modules = ['bitwriter', 'core', 'all-targets', 'x86', 'x86info']
llvm_version = ['>=14', '<16']
llvm_dep = dependency('llvm', method: 'config-tool', version: llvm_version, include_type: 'system', modules: llvm_modules)

elf_dep = dependency('libelf')
bfd_dep = meson.get_compiler('cpp').find_library('bfd', static: true)

extract_blob_source_files = files(
  'extract-blob-linker-map.cc',
  'extract-blob.cc',
  '../libirx/irinterpreter2-sections.cc',
)

extract_blob = executable('extract-blob', extract_blob_source_files, dependencies: [llvm_dep, elf_dep, bfd_dep])
