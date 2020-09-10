cmake_minimum_required(VERSION 2.8.8)
project(nepi_edge_sdk)

if(CMAKE_BUILD_TYPE STREQUAL "")
  message(WARNING "No CMAKE_BUILD_TYPE specified... setting to \"Debug\"")
  set(CMAKE_BUILD_TYPE "Debug")
endif()
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

###########
## Build ##
###########

## Specify source files
set(libc_src
  impl_c/nepi_edge_sdk_impl.c
  impl_c/nepi_lb_interface_impl.c
  impl_c/frozen/frozen.c
)

## Specify additional locations of header files
include_directories(
   include
   impl_c
)

## Declare an "object library" to avoid duplicate compilation
add_library(objlib OBJECT ${libc_src})
set_target_properties(objlib PROPERTIES LINKER_LANGUAGE C)

## Shared libraries need PIC
set_property(TARGET objlib PROPERTY POSITION_INDEPENDENT_CODE 1)

## Declare a static C library
add_library(${PROJECT_NAME}_static STATIC $<TARGET_OBJECTS:objlib>)
set_target_properties(${PROJECT_NAME}_static PROPERTIES LINKER_LANGUAGE C)

## Declare a dynamic C library
add_library(${PROJECT_NAME}_shared SHARED $<TARGET_OBJECTS:objlib>)
set_target_properties(${PROJECT_NAME}_shared PROPERTIES LINKER_LANGUAGE C)

## Build the examples
add_executable(nepi_sdk_example_session_c examples/c/nepi_sdk_example_session.c)
target_link_libraries(nepi_sdk_example_session_c
  ${PROJECT_NAME}_static
  -lm
)

#############
## Install ##
#############

## Declare the top-level directory for the install
set(INSTALL_DESTINATION ${CMAKE_SOURCE_DIR}/${CMAKE_BUILD_TYPE})

## Mark executables and/or libraries for installation
install(TARGETS ${PROJECT_NAME}_static ${PROJECT_NAME}_shared nepi_sdk_example_session_c
  ARCHIVE DESTINATION ${INSTALL_DESTINATION}/lib
  LIBRARY DESTINATION ${INSTALL_DESTINATION}/lib
  RUNTIME DESTINATION ${INSTALL_DESTINATION}/bin
  INCLUDES DESTINATION ${INSTALL_DESTINATION}/include
)

## Mark example python for installation
install(PROGRAMS examples/python/nepi_sdk_example_session.py
  DESTINATION ${INSTALL_DESTINATION}/bin
)

## Add the Python bindings to the lib subfolder
install(FILES python/nepi_edge_sdk.py
  DESTINATION ${INSTALL_DESTINATION}/lib
)

## Add the environment setup script the bin folder
install(FILES examples/examples_env.sh
  DESTINATION ${INSTALL_DESTINATION}/bin
)

install(DIRECTORY include
  DESTINATION ${INSTALL_DESTINATION}/include
)

install(DIRECTORY examples/nepi_sdk_example_filesys examples/example_files
  DESTINATION ${INSTALL_DESTINATION}/bin
)
