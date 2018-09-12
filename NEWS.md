# libsmp 0.6 release notes

## Highlights

* A major rework of the API has been done to support both static and dynamic
allocation. Now by default all objects are opaque and can't be allocated
directly by user but by using dedicated new()/free() function. To ease the
transition, a new and more general object has been introduced as a main entry
point for the SMP library: `SmpContext`. To keep supporting embedded platform
where dynamic allocation may not be suitable, a `static` API is available to
create object from user provided storage.

* `SmpSerialFrame` object and API have been deprecated. User shall use SmpContext
as a replacement. It will be removed in 0.8.0.

* Allocating SmpMessage on stack is deprecated. User shall use
`smp_message_new()` or `smp_message_new_from_static()` instead to get an
`SmpMessage` instance. The public definition of SmpMessage will be removed in
0.8.0.

* Error reporting is now done using a custom enumeration `SmpError` instead of
errno values which are not available on all supported platform.

## Bugs fixed in 0.6

* `fb50fce - fix(arduino): check that device exist in smp_serial_device_wait()`
* `cc66cca - fix(avr): check that device exist in smp_serial_device_wait()`
* `545a8bc - fix(serial_frame): test return value of serial_device_read() instead of errno`
* `334f57f - message: fix alignment issue for cpu without unaligned access support`:
    https://github.com/ActronikaSAS/libsmp/issues/24
* `52d8f4a - arduino: improve compatibility with Arduino Due and Teensy 3.6`:
    https://github.com/ActronikaSAS/libsmp/issues/20
