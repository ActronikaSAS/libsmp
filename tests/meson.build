tests_src = [
    'main.c',
    'serial_frame.c'
    ]

cunit_dep = dependency('cunit', version : '>= 2.0')

tests_exe = executable('tests', tests_src,
        dependencies : [libsmp_dep, cunit_dep])

test('tests', tests_exe)
