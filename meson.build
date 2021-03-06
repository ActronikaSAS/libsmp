project('smp', 'c', 'cpp',
    version : '0.9.0',
    meson_version : '>= 0.48.0',
    default_options : [
      'buildtype=debugoptimized',
      'warning_level=2'
      ])

c_compiler = meson.get_compiler('c')
cpp_compiler = meson.get_compiler('cpp')

pkgconfig = import('pkgconfig')

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
    'src/libsmp.c',
    'src/message.c',
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

# check size_t size
size = c_compiler.sizeof('size_t')
cdata.set('SMP_SIZE_T_SIZE', size)

cpu_with_unaligned_access_support = [ 'x86', 'x86_64', 'ppc', 'ppc64', 's390x' ]
if cpu_with_unaligned_access_support.contains(host_machine.cpu_family())
  cdata.set('HAVE_UNALIGNED_ACCESS', true)
endif

cdata.set('AVR_ENABLE_SERIAL0', get_option('avr-enable-serial0'),
    description : 'Enable AVR Serial0 handling')
cdata.set('AVR_ENABLE_SERIAL1', get_option('avr-enable-serial1'),
    description : 'Enable AVR Serial1 handling')
cdata.set('AVR_ENABLE_SERIAL2', get_option('avr-enable-serial2'),
    description : 'Enable AVR Serial2 handling')
cdata.set('AVR_ENABLE_SERIAL3', get_option('avr-enable-serial3'),
    description : 'Enable AVR Serial3 handling')
cdata.set('SMP_SERIAL_DEVICE_AVR_CBUF_SIZE',
    get_option('avr-uart-buffer-size').to_int(),
    description : 'The buffer size used for UART reception on AVR')
cdata.set('SMP_CONTEXT_PROCESS_CHUNK_SIZE', get_option('read-chunk-size'),
    description : 'Number of bytes processed by a single call of smp_context_process_fd')

configure_file(output : 'config.h',
    configuration: cdata)

libsmp_config_incdir = include_directories('.')

static_obj_list = [
    # objname, header file, token
    ['SmpBuffer', '"buffer.h"', 'smp-buffer-size'],
    ['SmpContext', '"context.h"', 'smp-context-size'],
    ['SmpSerialProtocolDecoder', '"serial-protocol.h"', 'smp-serial-protocol-decoder-size'],
    ['SmpMessage', '"libsmp-private.h"', 'smp-message-size'],
    ]

cdata = configuration_data()
foreach obj : static_obj_list
  args = [
      '-DSMP_DISABLE_LEGACY_API',
      # builddir (which contains libsmp-config.h) is not included when checking
      # sizeof so:
      '-I' + meson.current_build_dir(),
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
    output: 'libsmp-static.h',
    configuration: cdata,
    install_dir: get_option('includedir'))


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
    dependencies : dependencies,
    install: true)

libsmp_dep = declare_dependency(link_with : libsmp,
    include_directories : [libsmp_incdir, libsmp_config_incdir],
    dependencies : dependencies)

install_headers(['include/libsmp.h'])

pkgconfig.generate(libsmp,
    name: 'smp',
    description: 'Simple Message Protocol library',
    version: meson.project_version(),
    filebase: 'libsmp',
    url: 'https://github.com/ActronikaSAS/libsmp'
    )

subdir('docs')
subdir('tests')
