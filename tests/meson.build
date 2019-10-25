if get_option('tests').disabled()
  subdir_done()
endif

tests_src = [
    'context.c',
    'main.c',
    'message.c',
    'serial-protocol.c',
    ]

cunit_dep = dependency('cunit', version : '>= 2.0', required : false)

if (cunit_dep.found())
  tests_exe = executable('tests', tests_src,
      include_directories: include_directories('../src'),
      c_args: ['-DSMP_DISABLE_DEPRECATED'],
      dependencies : [libsmp_dep, cunit_dep])

  test('tests', tests_exe)
else
  message('CUnit not found: disabling tests')
endif
