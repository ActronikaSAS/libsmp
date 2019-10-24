==============
 Installation
==============

Requirements
============

Mandatory
---------

* `mesonbuild <https://mesonbuild.com>`_
* `ninja <https://ninja-build.org/>`_

Optional
--------

* docs:

  * `doxygen <http://www.doxygen.org>`_
  * `sphinx <http://www.sphinx-doc.org>`_
  * `breathe <https://github.com/michaeljones/breathe>`_
  * `Read the Docs Sphinx Theme <https://github.com/rtfd/sphinx_rtd_theme>`_

* tests:

  * `cunit <http://cunit.sourceforge.net/>`_

Building
========

The build is rather simple and need only a few commands:

.. code::

   $ meson build
   $ cd build
   $ ninja
   $ ninja install

Configuration
=============

The build configuration could be displayed using ``meson configure`` command.
Options could be set by adding ``-D<option-name>=<value>`` to it.
For instance, to explicitely disable ``docs`` building:

.. code::

   $ meson configure -Ddocs=disabled


.. _arduino-library:

Arduino library
===============

It is possible to export this library for the Arduino IDE. To do so, run the
script ``export-arduino-lib.py`` located in the ``scripts`` directory.
It will generate an archive which have to be imported within the IDE.
