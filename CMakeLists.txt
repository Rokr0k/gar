cmake_minimum_required(VERSION 3.23)

project(
  gar
  VERSION 0.0.1
  LANGUAGES C)

find_package(zstd CONFIG REQUIRED)
find_package(xxHash CONFIG REQUIRED)

configure_file("config.h.in" "config.h")
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(gar SHARED "src/gar.c" "src/fmap.c" "src/reader.c" "src/writer.c")
target_sources(
  gar
  PUBLIC FILE_SET
         HEADERS
         BASE_DIRS
         "include"
         FILES
         "include/gar/gar.h"
         "include/gar/exports.h"
         "include/gar/reader.h"
         "include/gar/writer.h"
         "include/gar/gar.hpp"
         "include/gar/reader.hpp"
         "include/gar/writer.hpp")
target_link_libraries(gar PRIVATE zstd::libzstd xxHash::xxhash)
target_compile_features(gar PUBLIC c_std_11)
target_compile_definitions(gar PRIVATE _CRT_SECURE_NO_WARNINGS)
set_target_properties(gar PROPERTIES VERSION ${PROJECT_VERSION}
                                     SOVERSION ${PROJECT_VERSION_MAJOR})

add_executable(gartool "tool/main.c")
target_link_libraries(gartool gar)
target_compile_features(gartool PUBLIC c_std_11)
target_compile_definitions(gartool PRIVATE _CRT_SECURE_NO_WARNINGS)
set_target_properties(gartool PROPERTIES OUTPUT_NAME gar)

install(
  TARGETS gar gartool
  EXPORT garTargets
  LIBRARY
  RUNTIME FILE_SET HEADERS)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "garConfig.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/garConfig.cmake"
  INSTALL_DESTINATION "lib/cmake/gar")
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/garConfigVersion.cmake"
  COMPATIBILITY SameMajorVersion)

install(
  EXPORT garTargets
  NAMESPACE gar::
  DESTINATION "lib/cmake/gar")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/garConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/garConfigVersion.cmake"
        DESTINATION "lib/cmake/gar")
