========
 libsmp
========

libsmp stands for Simple Message Protocol library. It aims to provide a simple
protocol to exchange message between two peers over a serial lane.

Messages are identified by an ID, choosen by user application, and can
contains 0 or more arguments which are typed.

Supported platforms
===================

* Arduino (see :ref:`arduino-library`)
* AVR (see :ref:`avr-port`)
* GNU/Linux (Posix systems should works as well)
* Windows

.. toctree::
   :caption: Usage
   :maxdepth: 1

   installation
   static-api
   avr-port

.. toctree::
   :caption: Protocol specifications
   :maxdepth: 1

   protocols/message-protocol
   protocols/serial-protocol

.. toctree::
   :caption: API Reference
   :maxdepth: 1
   :glob:

   api/*

.. toctree::
   :caption: Appendix
   :maxdepth: 1

   genindex
