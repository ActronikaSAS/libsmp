tests_src = [
    'main.c',
    'message.c',
    'serial_frame.c'
    ]

cunit_dep = dependency('cunit', version : '>= 2.0', required : false)

if (cunit_dep.found())
  tests_exe = executable('tests', tests_src,
          dependencies : [libsmp_dep, cunit_dep])

  test('tests', tests_exe)
else
  message('CUnit not found: disabling tests')
endif
