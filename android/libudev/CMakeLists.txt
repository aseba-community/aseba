cmake_minimum_required(VERSION 2.6)

project(libudev)

include_directories(src/src src/src/shared src/src/systemd)

add_definitions(-DO_CLOEXEC=0 -DO_NOFOLLOW=0 -D_GNU_SOURCE
  -DHAVE_SECURE_GETENV)
file(GLOB LIBUDEV_SOURCES src/src/libudev/*.c)
add_library(udev ${LIBUDEV_SOURCES})
