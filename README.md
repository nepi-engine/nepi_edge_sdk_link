<!--
Copyright (c) 2024 Numurus, LLC <https://www.numurus.com>.

This file is part of nepi-engine
(see https://github.com/nepi-engine).

License: 3-clause BSD, see https://opensource.org/licenses/BSD-3-Clause
-->
# nepi_edge_sdk_link

This repository provides an SDK for NEPI-Connect/NEPI-Link for Edge Devices. It allows applications to control nepi-bot (edge-side interface for NEPI-Connect).

It is used extensively by the [nepi_link_ros_bridge](https://bitbucket.org/numurus/nepi_link_ros_bridge/src/master/) node. Other applications may find it useful as well.

Provides both a C-language library and Python module (2.7-compatible, 3.x-preferred).
Example files within this repository demonstrate usage.

## Building
This is a CMake project, so standard CMake build patterns apply

      mkdir build && cd build
      cmake -DCMAKE_BUILD_TYPE=Release ..
      make

The CMAKE_BUILD_TYPE defaults to _Debug_, so if optimization is desired, you must
provide -DCMAKE_BUILD_TYPE=Release as above.

## Installing
The CMake file checks for existence of a INSTALL_DESTINATION variable that Specifies
the absolute path to an install directory. If that variable is omitted, the _install_
is to a local folder that matches the CMAKE_BUILD_TYPE value (default = _Debug_). 

> **Note: We strongly suggest using install location /opt/nepi/nepi_link/nepi_edge_sdk_link for easiest interoperability with other NEPI components.**
For example,

      cmake -DCMAKE_BUILD_TYPE=Release -DINSTALL_DESTINATION=/opt/nepi/nepi_link/nepi_edge_sdk_link ..
      make install

will build and install an optimized package to the suggested directory.

## Using the library
Examples are provided in the _bin_ folder of the installation.

By default both a dynamic and a static C library are built and included in the
_lib_ folder of the install.

### Runtime Dependencies
As a convenience, the runtime dependencies can be loaded into your environment
via the _setup.bash_ file in the root of the installation folder.

      $ source /my/installation/folder/setup.bash

will prepare your shell to run applications that leverage either the C or the Python
version of the SDK.

Here are the details:
To use the C dynamic library, the loader search path must include the _lib_ folder of this
install. This can be specified as a flag to the linker or provided through the
_LD_LIBRARY_PATH_ environment variable. Alternatively, the dynamic library can be
simply copied into the run folder of your application.

To use the Python bindings, your Python interpreter must be able to find _nepi_edge_sdk_link.py_.
You can update your PYTHONPATH environment variable or simply copy this file into the
same folder as your main Python application. Additionally, your loader must be able to
find the C dynamic library as described above.
