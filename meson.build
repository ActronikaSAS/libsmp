project('smp', 'c', 'cpp',
    version : '0.3.0',
    meson_version : '>= 0.38.0',
    default_options : [
      'warning_level=2'
      ])

if meson.get_compiler('c').get_id() == 'gcc'
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
endif

libsmp_incdir = include_directories(['include'])

libsmp_src = [
    'src/message.c',
    'src/serial_frame.c'
    ]

# select SerialDevice implementation depending on cpu family and stack perferencies
if get_option('use-arduino-lib')
    libsmp_src += ['src/serial-device-arduino.cpp']
    add_project_arguments(['-Wno-non-virtual-dtor'], language : 'cpp')
else
    if host_machine.cpu_family() == 'atmega'
        libsmp_src += ['src/serial-device-avr.c']
    else
        if host_machine.system() == 'windows'
            libsmp_src += ['src/serial-device-win32.c']
        else
            libsmp_src += ['src/serial-device-posix.c']
        endif
    endif
endif

## Private configuration
cdata = configuration_data()

# check for termios header
if meson.get_compiler('c').has_function('tcgetattr', prefix: '#include <termios.h>')
  cdata.set('HAVE_TERMIOS_H', true)
endif

cdata.set('AVR_ENABLE_SERIAL0', get_option('avr-enable-serial0'),
    description : 'Enable AVR Serial0 handling')
cdata.set('AVR_ENABLE_SERIAL1', get_option('avr-enable-serial1'),
    description : 'Enable AVR Serial1 handling')
cdata.set('AVR_ENABLE_SERIAL2', get_option('avr-enable-serial2'),
    description : 'Enable AVR Serial2 handling')
cdata.set('AVR_ENABLE_SERIAL3', get_option('avr-enable-serial3'),
    description : 'Enable AVR Serial3 handling')

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

if get_option('use-arduino-lib')
    arduino_core_dep = subproject('arduino-core').get_variable('arduino_core_dep')
    dependencies = [arduino_core_dep]
else
    dependencies = []
endif

libsmp_flags = []
if get_option('default_library') == 'shared'
  libsmp_flags += '-DSMP_EXPORT_API'
endif

libsmp = library('smp', libsmp_src,
    include_directories : [libsmp_incdir, libsmp_config_incdir],
    c_args : libsmp_flags,
    cpp_args : libsmp_flags,
    dependencies : dependencies)

libsmp_dep = declare_dependency(link_with : libsmp,
    include_directories : [libsmp_incdir, libsmp_config_incdir],
    dependencies : dependencies)

subdir('tests')
