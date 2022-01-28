cmake_minimum_required(VERSION 3.4.1)
project(zygisk_mgl)

include_directories(.)

find_package(cxx REQUIRED CONFIG)
find_package(nativehelper REQUIRED CONFIG)
find_package(xhook REQUIRED CONFIG)

add_library(zygisk_mgl SHARED main.cpp hook.cpp android.cpp misc.cpp config.cpp tinynew.cpp)
target_link_libraries(zygisk_mgl log cxx::cxx nativehelper::nativehelper_header_only xhook::xhook)
