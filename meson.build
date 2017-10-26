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

libsmp_incdir = include_directories(['include'])

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

## Private configuration
cdata = configuration_data()

# check for termios header
if meson.get_compiler('c').has_function('tcgetattr', prefix: '#include <termios.h>')
  cdata.set('HAVE_TERMIOS_H', true)
endif

configure_file(output : 'config.h',
    configuration: cdata)

## Public config file
cdata = configuration_data()

cdata.set('SMP_MESSAGE_MAX_VALUES', get_option('message-max-values').to_int(),
    description : 'The maximum numbers of arguments in a message')
cdata.set('SMP_SERIAL_FRAME_MAX_FRAME_SIZE',
    get_option('serial-frame-max-size').to_int(),
    description : 'The maximum frame size in bytes')
cdata.set('SMP_SERIAL_DEVICE_AVR_CBUF_SIZE',
    get_option('avr-uart-buffer-size').to_int(),
    description : 'The buffer size used for UART reception on AVR')

configure_file(output : 'libsmp-config.h',
    configuration : cdata)

libsmp_config_incdir = include_directories('.')

libsmp = library('smp', libsmp_src,
    include_directories : [libsmp_incdir, libsmp_config_incdir])

libsmp_dep = declare_dependency(link_with : libsmp,
    include_directories : [libsmp_incdir, libsmp_config_incdir])

subdir('tests')
