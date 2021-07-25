cmake_minimum_required(VERSION 3.0)
project(lews VERSION 1.0.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)

set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS")
IF(MSVC)
    SET(CMAKE_CXX_FLAGS "/EHsc")
ENDIF(MSVC)

set(SOURCE
  base64.cpp
  base64.h
  connection.cpp
  connection.h
  frame.cpp
  frame.h
  tools.h
  websocket.cpp
  websocket.h
  md5/md5.h
  md5/md5.cpp
  demo/demo.h
  demo/demo.cpp
  demo/user.h
  demo/user.cpp
)

if(WIN32)
add_definitions(/DWIN32 /DWIN32_LEAN_AND_MEAN /D_UNICODE /DUNICODE)
endif()

if(WIN32)
set(SYSTEM_LIB
  WS2_32
)
else()
if(APPLE)
set(SYSTEM_LIB
  "-framework Foundation"
  "-framework CoreFoundation"
  "-framework AppKit"
  "-framework IOKit"
  "-framework AVFoundation"
)
else()
set(SYSTEM_LIB
)
endif()
endif()

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE} ${SOURCE_LIB})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${SYSTEM_LIB})

find_package(Libevent CONFIG REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE libevent::core libevent::extra libevent::openssl)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE MATCHES "Release")
  target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /Zi)
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES 
    LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
  )
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES 
    COMPILE_PDB_NAME ${CMAKE_PROJECT_NAME}
    COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
  )
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "${CMAKE_PROJECT_NAME}")
