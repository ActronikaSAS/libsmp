.. _avr-port:

==========
 AVR port
==========

AVR port is quite special as we don't have any OS on this platform. The serial
device directly implements a driver for AVR UARTs modules. AVR port currently
supports the following chips:

* Atmega 328p
* Atmega 2560

As these system don't have much RAM, you should tweak all buffers size using
meson configure tool.

The serial device name have the following format ``serialX`` with X a number from
0 to the maximum UART device. For instance, the Atmega 328p has only one uart
device which is named ``serial0``.

The returned fd on this platform is not a file descriptor at all so it shouldn't
be use outside of smp functions.
