# Build the zephyr/scripts/native_simulator/ library in LLVM-IR
# The library is required for zephyr apps build for native_sym
# The interpreter will load this library to resolve its external symbols

make = find_program('make')
toolchains = subproject('toolchains', required: false)
zephyr = subproject('zephyr', required: false)
gllvm = subproject('gllvm-meson', required: false)

if toolchains.found() and zephyr.found() and gllvm.found()
    zephyr_dir = zephyr.get_variable('source_dir')
    native_simulator_source_dir = zephyr_dir / 'scripts' / 'native_simulator'
    
    llvm_bindir = toolchains.get_variable('llvm_bindir')
    llvm_link = llvm_bindir / 'llvm-link'
    llvm_ar = toolchains.get_variable('llvm_ar')
    
    gclang = gllvm.get_variable('gclang')
    get_bc = gllvm.get_variable('get_bc')
    
    env = {'LLVM_COMPILER_PATH': llvm_bindir} # set the llvm path for gclang    
    
    native_simulator_library = custom_target('zephyr_native_simulator_library',
      output: ['runner.a'],
      command: [make, '-C', native_simulator_source_dir, 'runner_lib'],
      env: env + {
        'CCACHE_DISABLE': '1', # builds fast anyway
        'NSI_CC': gclang.full_path(),
        'NSI_AR': llvm_ar.full_path(),
        'NSI_PATH': native_simulator_source_dir / '', # Makefile requires tailing slash
        'NSI_BUILD_PATH': meson.current_build_dir(),
        'NSI_NATIVE': '1',
        'NSI_COVERAGE': '',
        'NSI_CPPFLAGS': '-D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=600 -D_XOPEN_SOURCE_EXTENDED' + ' -DNSI_NO_MAIN -fno-discard-value-names',
      }
    )
    
    native_simulator_bca = custom_target('zephyr_native_simulator_bca',
      output: ['runner.bca'],
      input: native_simulator_library,
      command: [get_bc, '-o', '@OUTPUT@', '@INPUT@'],
      env: env
    )
    
    native_simulator_llvmir = custom_target('zephyr_native_simulator_llvmir',
      output: ['runner.ll'],
      input: native_simulator_bca,
      command: [llvm_link, '-S', '-o', '@OUTPUT@', '@INPUT@'],
    )
    
    nop = find_program('true')
    run_target('native_simulator', command: nop, depends: native_simulator_llvmir)
endif
