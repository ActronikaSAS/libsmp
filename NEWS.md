# libsmp 0.8 release notes

## Highlights

* As it was planned, the `SmpSerialFrame` has been removed and the `SmpMessage`
definition has been made private.

* The `SmpMessage` now dynamically allocate internal values storage instead of
using a compile-time configuration. User can set the capacity of a message using
`smp_message_set_capacity()` function and he should ensure that it is large
enough to hold the message he wants to build. There is no reallocation except
when receiving a message.

* The continous integration has been moved from a Jenkins job configuration to
a pipeline described in the `JenkinsFile`.

* Teensy 3.5 support has been added in Arduino serial device.

* Library now detects when a device has been removed on Posix and Windows
platform and return an appropriate error to the application.

* Documentation has been moved from Doxygen and text file to Sphinx and Breathe.

## Bugs fixed in 0.8

* `4134ad8 - Add missing SMP_API to smp_error_to_string()`
* `078b976 - context: fix serial protocol decoded memory leak`
* `6b5ce44 - Guard SMP_ENABLE_STATIC_API against redefinition`
* `dea4c53 - buffer: define SMP_ENABLE_STATIC_API to compile for Arduino targets`
* `c6c0354 - context: define SMP_ENABLE_STATIC_API to compile for Arduino targets`
* `91d8077 - avr: use SmpSerialBaudrate instead of SmpSerialFrameBaudrate`
* `aca482b - serial_frame: cast to int to avoid msvc warning about possible loss of data`
* `137a39f - context: fix possible loss of data warnings emitted by msvc`
* `df7cedf - message: return in error when we have a loss of data due to downcasting`
* `3522fd3 - serial-device-win32: check that size can fit in a DWORD in read()/write()`
* `3f2c41c - message: cast va_arg return when type differs from storage`
* `06138c5 - serial-device-win32: don't negate error code`
* `90ddc15 - serial-device: add an init function`
* `86adb12 - serial-device: reset fd/handle to an invalid on close`
* `38c7778 - message: set statically_allocated flag accordingly`
* `d12930f - context: check that context was not allocated using from_static() before free`
* `6859c91 - serial-protocol: set SMP_ENABLE_STATIC_API in source file instead of header`
* `8101ebc - serial-protocol: enable static API and remove StaticSerialProtocolDecoder typedef`
* `c088a9e - Add extern "C" in all header when compiling with a c++ compiler`
* `0eff109 - context: clear message after calling new_message_cb`
* `de310ba - message: fix overflow when checking for string length in encode_value()`
