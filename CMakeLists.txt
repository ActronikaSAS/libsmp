# Add source files
set(src_files "src/buffer.c"
              "src/context.c"
              "src/libsmp.c"
              "src/message.c"
              "src/serial-protocol.c"
              "src/serial-device-stubs.c"
)

# Add include directory
set(inc_dir "src" "include")

idf_component_register(SRCS ${src_files}
                       INCLUDE_DIRS ${inc_dir})

# Set compiler definitions
target_compile_definitions(${COMPONENT_LIB} PRIVATE "-DSMP_DISABLE_LEGACY_API"
                                                    "-DSMP_ENABLE_STATIC_API"
                                                    "-DSMP_DISABLE_DEPRECATED")