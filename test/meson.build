envdata = environment()
python_paths = [join_paths(meson.current_build_dir(), '..')]
# python_paths = [join_paths(meson.current_source_dir(), '..')]
envdata.append('PYTHONPATH', python_paths)
envdata.append('TESTS_BUILDDIR', meson.current_build_dir())
test(
  'python-unittests',
  import('python').find_installation('python3'),
  args: ['-m', 'unittest'],
  env: envdata,
  workdir: join_paths(meson.current_source_dir(), '..')
)
