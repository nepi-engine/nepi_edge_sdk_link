#nepi-edge-sdk

Description: SDK for Numurus NEPI Edge Devices

Provides both a C-language library and Python module (2.7-compatible, 3.x-preferred).
Example files show usage.

## Building
This is a CMake project, so standard CMake build patterns apply

      mkdir build && cd build
      cmake -DCMAKE_BUILD_TYPE=Release
      make

The CMAKE_BUILD_TYPE defaults to _Debug_, so if optimization is desired, you must
provide the -DCMAKE_BUILD_TYPE=Release as above.

## Installing
The CMake file checks for existence of a INSTALL_DESTINATION variable that Specifies
the absolute path to an install directory. If that variable is omitted, the _install_
is to a local folder that matches the CMAKE_BUILD_TYPE value (default = _Debug_). For example,

      cmake -DCMAKE_BUILD_TYPE=Release -DINSTALL_DESTINATION=/my/favorite/folder
      make install

will build and install an optimized package to /my/favorite/folder

## Using the library
Examples are provided in the _bin_ folder of the installation.

By default both a dynamic and a static C library are built and included in the
_lib_ folder of the install.

### Runtime Dependencies
To use the C dynamic library, the loader search path must include the _lib_ folder of this
install. This can be specified as a flag to the linker or provided through the
_LD_LIBRARY_PATH_ environment variable. Alternatively, the dynamic library can be
simply copied into the run folder of your application.

To use the Python bindings, your Python interpreter must be able to find nepi_edge_sdk.py.
You can update your PYTHONPATH environment variable or simply copy this file into the
same folder as your main Python application. Additionally, your loader must be able to
find the C dynamic library as described above.
