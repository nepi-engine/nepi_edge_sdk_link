<!--
NEPI Dual-Use License
Project: nepi_edge_sdk_link

This license applies to any user of NEPI Engine software

Copyright (C) 2023 Numurus, LLC <https://www.numurus.com>
see https://github.com/numurus-nepi/nepi_edge_sdk_link

This software is dual-licensed under the terms of either a NEPI software developer license
or a NEPI software commercial license.

The terms of both the NEPI software developer and commercial licenses
can be found at: www.numurus.com/licensing-nepi-engine

Redistributions in source code must retain this top-level comment block.
Plagiarizing this software to sidestep the license obligations is illegal.

Contact Information:
====================
- https://www.numurus.com/licensing-nepi-engine
- mailto:nepi@numurus.com

-->
# nepi_edge_sdk_link

Description: SDK for NEPI-Connect/NEPI-Link for Edge Devices

Provides both a C-language library and Python module (2.7-compatible, 3.x-preferred).
Example files show usage.

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
is to a local folder that matches the CMAKE_BUILD_TYPE value (default = _Debug_). For example,

      cmake -DCMAKE_BUILD_TYPE=Release -DINSTALL_DESTINATION=/my/favorite/folder ..
      make install

will build and install an optimized package to _/my/favorite/folder_

> Note: Suggest using install location /opt/nepi/nepi_link/nepi_edge_sdk_link for easiest interoperability with other NEPI components.

## Using the library
Examples are provided in the _bin_ folder of the installation.

By default both a dynamic and a static C library are built and included in the
_lib_ folder of the install.

### Runtime Dependencies
As a convenience, the runtime dependencies can be loaded into your environment
via the _setup.bash_ file in the root of the installation folder.

      source /my/installation/folder/setup.bash

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
