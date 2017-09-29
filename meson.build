project('smp', 'c',
    version : '0.1.0',
    meson_version : '>= 0.38.0',
    default_options : [
      'warning_level=2'
      ])

add_project_arguments([
    # warnings flags
    '-Wno-unused-parameter',
    '-Wformat-nonliteral',
    '-Wformat-security',
    '-Winit-self',
    '-Wswitch-default',
    '-Wuninitialized',
    '-Wmaybe-uninitialized',
    '-Wshadow',
    '-Wframe-larger-than=4096',
    '-Wpointer-arith',
    '-Wundef',
    '-Wcast-qual',
    '-Wjump-misses-init',
    '-Wmissing-prototypes',
    '-Wno-type-limits'
    ], language : 'c')


libsmp_src = [
    'src/message.c',
    'src/serial_frame.c',
    ]

libsmp_incdir = include_directories(['include'])

libsmp = library('smp', libsmp_src,
    include_directories : [libsmp_incdir])

libsmp_dep = declare_dependency(link_with : libsmp,
    include_directories : [libsmp_incdir])

subdir('tests')
