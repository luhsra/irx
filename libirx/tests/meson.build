# cmake = import('cmake')
# gtest_proj = cmake.subproject('gtest')

gtest = dependency('gtest', main: true, disabler: true, required: get_option('tests'))
gmock = dependency('gmock', disabler: true, required: get_option('tests'))

#     llvm_modules = ['mcjit', 'interpreter', 'executionengine', 'irreader', 'linker']
#     llvm_version = ['>=9', '<16']
#     llvm_dep = dependency('llvm', method: 'config-tool', version: llvm_version, include_type: 'system', modules: llvm_modules)

llvm_bindir = llvm_dep.get_variable(configtool: 'bindir')
clang = find_program(llvm_bindir / 'clang')

infiles = ['../test-appl/simple/test.cc', '../test-appl/simple/test-extern.cc', '../test-appl/dlist/dlist.cc']
clang_cargs = []
# clang_cargs += ['-m32'] # tests depends on 64-bit llvm-ir
llvmir_files = []
foreach infile : infiles
  llvmir_files += custom_target('llvm-ir-compile-' + infile.underscorify(),
    output  : '@BASENAME@.ll',
    input : infile,
    command : [clang, '-o', '@OUTPUT@', '-S', '-emit-llvm', '-g3', clang_cargs, '@INPUT@'])
endforeach

if gtest.found() and gmock.found()
    gtest_source_files = files(
      '../irinterpreter2-debug.cc',
      '../irinterpreter2-struct.cc',
      '../irinterpreter2-global.cc',
      '../irinterpreter2-logging.cc',
      '../irinterpreter2-mmap.cc',
      '../irinterpreter2-interpreter.cc',
      '../irinterpreter2-sections.cc',
    )

    deps = [llvm_dep, gtest, gmock]

    # if gtest is used from subprojects as a fallback it should link to gtest_main
    gtest_main = dependency('gtest_main', required: true)
    if gtest_main.found()
      deps += [gtest_main]
    endif

    gtests = [
      'test',
      'sections',
      'gep',
    ]

    foreach t : gtests
        e = executable('gtest_' + t, gtest_source_files + [t + '.cc'], dependencies: deps)
        test('gtest', e, depends: llvmir_files)
        #args : ['--llvm-ir-files' + llvmir_files.join(',') ]
    endforeach
endif
