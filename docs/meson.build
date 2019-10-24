if get_option('doc').disabled()
  subdir_done()
endif

docfiles = [
  '_static/theme_overrides.css',
  'api/buffer.rst',
  'api/context.rst',
  'api/error.rst',
  'api/message.rst',
  'api/serial-protocol.rst',
  'avr-port.rst',
  'index.rst',
  'installation.rst',
  'protocols/message-protocol.rst',
  'protocols/serial-protocol.rst',
  'static-api.rst',
  ]

# Doxygen check
doxygen = find_program('doxygen', required: false)
if not doxygen.found()
  message('Doxygen not found, disabling documentation')
  subdir_done()
endif

# Sphinx check
sphinx = find_program('sphinx-build', required: false)
if not sphinx.found()
  message('Sphinx not found, disabling documentation')
  subdir_done()
endif

# Checking Breathe presence by searching for breathe-apidoc executable
breathe_apidoc = find_program('breathe-apidoc', required: false)
if not breathe_apidoc.found()
  message('Breathe not found, disabling documentation')
  subdir_done()
endif

cdata = configuration_data()
cdata.set('version', meson.project_version())
cdata.set('rootdir', meson.source_root())
cdata.set('builddir', meson.build_root())

# generate doxygen configuration file
doxyfile = configure_file(input: 'Doxyfile.in',
  output: 'Doxyfile',
  configuration: cdata)

# generate sphinx configuration file
conf = configure_file(input: 'conf.py.in',
  output: 'conf.py',
  configuration: cdata)

srcdir = join_paths(meson.source_root(), 'docs', 'source')
docdir = join_paths(get_option('datadir'), 'doc', 'libsmp')

doxygen_target = custom_target('docs-doxygen',
  input: doxyfile,
  output: 'xml',
  command: [doxygen, doxyfile],
  depends: [libsmp],
  install: false)

# make sphinx target also depends on libs as the doxygen target will generate
# its output in a directory but this directory timestamp is not updated.
sphinx_target = custom_target('docs-sphinx',
  input: conf,
  output: 'html',
  command: [sphinx,
    '-c', '@OUTDIR@',
    meson.source_root() + '/docs/',
    '@OUTPUT@'],
  depends: [doxygen_target, libsmp],
  depend_files: docfiles,
  install: true,
  install_dir: docdir)
