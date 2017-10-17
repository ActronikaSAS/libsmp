# libsmp

libsmp stands for Simple Message Protocol library. It aims to provide a simple
protocol to exchange message between two peers over a serial lane.
Messages are identified by an ID, choosen by user application, and contains 0 or
more arguments which are typed.

Protocols are documented in the docs directory.

## Getting started

### Requirements

* Meson build system and ninja

### Compiling

```bash
$ meson build
$ cd build/
$ ninja
```
