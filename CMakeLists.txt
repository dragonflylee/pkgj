cmake_minimum_required(VERSION 3.6)

project(pkgj)

if(NOT HOST_BUILD)
  if(NOT BUILD_VITA)
    message(FATAL_ERROR "Vita toolchain not imported, fix the conan recipe")
  endif()

  include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
endif()

# useful for memory alignment constraints
set(CMAKE_CXX_STANDARD 17)

set(VITA_APP_NAME "PKGj")
set(VITA_TITLEID  "PKGJ00002")
set(VITA_VERSION  "0.59")

option(PKGI_ENABLE_LOGGING "enables debug logging over udp multicast" OFF)

if(PKGI_ENABLE_LOGGING)
  add_definitions(-DPKGI_ENABLE_LOGGING)
endif()

add_definitions(-DPKGI_VERSION="${VITA_VERSION}" -D_GNU_SOURCE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fvisibility=hidden")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -fvisibility=hidden")

find_package(fmt REQUIRED)
find_package(Boost REQUIRED)
find_package(cereal REQUIRED)
find_package(libzip REQUIRED)

if(HOST_BUILD)
  include(host.cmake)
else()
  include(cross.cmake)
endif()
