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
    'src/serial_frame.c'
    ]

# select SerialDevice implementation depending on cpu family
if host_machine.cpu_family() == 'atmega'
  libsmp_src += ['src/serial-device-avr.c']
else
  libsmp_src += ['src/serial-device-posix.c']
endif


cdata = configuration_data()

# check for termios header
if meson.get_compiler('c').has_function('tcgetattr', prefix: '#include <termios.h>')
  cdata.set('HAVE_TERMIOS_H', true)
endif

configure_file(output : 'config.h',
    configuration: cdata)

libsmp_incdir = include_directories(['include'])

libsmp = library('smp', libsmp_src,
    include_directories : [libsmp_incdir])

libsmp_dep = declare_dependency(link_with : libsmp,
    include_directories : [libsmp_incdir])

subdir('tests')
