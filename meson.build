project('smp', 'c', 'cpp',
    version : '0.5.90.1',
    meson_version : '>= 0.43.0',
    default_options : [
      'warning_level=2'
      ])

c_compiler = meson.get_compiler('c')
cpp_compiler = meson.get_compiler('cpp')

common_warning_flags = [
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
    '-Wno-type-limits'
    ]

c_warning_flags = common_warning_flags
c_warning_flags += [
    '-Wjump-misses-init',
    '-Wmissing-prototypes',
    ]

cpp_warning_flags = common_warning_flags

libsmp_incdir = include_directories(['include'])

libsmp_src = [
    'src/buffer.c',
    'src/context.c',
    'src/message.c',
    'src/serial_frame.c',
    'src/serial-protocol.c',
    ]

# select SerialDevice implementation depending on cpu family and stack perferencies
if get_option('use-arduino-lib')
    libsmp_src += ['src/serial-device-arduino.cpp']
    cpp_warning_flags += ['-Wno-non-virtual-dtor']
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
if c_compiler.has_function('tcgetattr', prefix: '#include <termios.h>')
  cdata.set('HAVE_TERMIOS_H', true)
endif

# check for poll
if c_compiler.has_function('poll', prefix: '#include <poll.h>')
  cdata.set('HAVE_POLL_H', true)
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

static_obj_list = [
    # objname, header file, token
    ['SmpBuffer', '"buffer.h"', 'smp-buffer-size'],
    ['SmpContext', '"context.h"', 'smp-context-size'],
    ['SmpSerialProtocolDecoder', '"serial-protocol.h"', 'smp-serial-protocol-decoder-size'],
    ['SmpMessage', '"libsmp.h"', 'smp-message-size'],
    ]

cdata = configuration_data()
foreach obj : static_obj_list
  args = [
      '-DSMP_DISABLE_LEGACY_API',
      # builddir (which contains libsmp-config.h) is not included when checking
      # sizeof so:
      '-I' + meson.current_build_dir()
      ]

  incdir = [
      libsmp_incdir,
      include_directories('src'),
      libsmp_config_incdir,
      ]

  size = c_compiler.sizeof(obj[0],
      prefix: '#include ' + obj[1],
      include_directories: incdir,
      args: args)

  if size <= 0
    error('failed to determine ' + obj[0] + ' size')
  endif

  cdata.set(obj[2], size)
endforeach
configure_file(input: 'src/libsmp-static.h.in',
    output: 'libsmp-static.h', configuration: cdata)


dependencies = []
if get_option('use-arduino-lib')
  arduino_core_dep_list = [
      # dep name, subproject name, dep variable
      ['arduino-core', 'arduino-core', 'arduino_core_dep'],
      ['teensy-core', 'teensy-core', 'teensy_core_dep'],
      ]

  # we have no break in Meson right now so do it by hand
  found = false
  foreach dep : arduino_core_dep_list
    if not found
      arduino_core_dep = dependency(dep[0],
          fallback: [dep[1], dep[2]],
          required: false)
      if arduino_core_dep.found()
        found = true
      endif
    endif
  endforeach

  if not arduino_core_dep.found()
    error('No arduino core found')
  endif

  dependencies += [arduino_core_dep]
endif

# add warnings flags
add_project_arguments(c_compiler.get_supported_arguments(c_warning_flags),
    language: 'c')
add_project_arguments(cpp_compiler.get_supported_arguments(cpp_warning_flags),
    language: 'cpp')

libsmp_flags = ['-DSMP_ENABLE_STATIC_API',
    '-DSMP_DISABLE_DEPRECATED']
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
