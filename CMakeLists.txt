cmake_minimum_required(VERSION 3.16)

project(adaptive-resolution-object-detection LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(APP_NAME adaptive-resolution-object-detector)

# ---------------------------------------------------------------------------
# NEAT core.
#
# Installing the NEAT SDK puts SimaNeatConfig.cmake in the sysroot, so the
# config package is the normal path. Cross builds find it through the
# toolchain file; host builds find it wherever the SDK was installed. Point
# CMAKE_PREFIX_PATH at the install tree if it lands somewhere unusual.
# ---------------------------------------------------------------------------
find_package(SimaNeat CONFIG REQUIRED)

if (TARGET SimaNeat::sima_neat_shared)
  set(NEAT_TARGET SimaNeat::sima_neat_shared)
elseif (TARGET SimaNeat::sima_neat)
  set(NEAT_TARGET SimaNeat::sima_neat)
else()
  message(FATAL_ERROR
    "SimaNeat was found but exports no usable target. "
    "Expected SimaNeat::sima_neat_shared or SimaNeat::sima_neat.")
endif()

find_package(nlohmann_json REQUIRED)

# ---------------------------------------------------------------------------
# Shared helpers: argument and config parsing, RTSP probing, NV12 conversion,
# and detection-metadata packing.
# ---------------------------------------------------------------------------
add_library(app_support STATIC
  support/runtime/config_utils.cpp
  support/runtime/example_utils.cpp
  support/object_detection/obj_detection_utils.cpp
)

target_link_libraries(app_support
  PUBLIC
    ${NEAT_TARGET}
    nlohmann_json::nlohmann_json
)

# Repo root, so support sources resolve their own "support/..." includes.
target_include_directories(app_support PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# ---------------------------------------------------------------------------
# The detector. One binary, two topologies, selected at runtime by --mode.
# ---------------------------------------------------------------------------
add_executable(${APP_NAME} src/cpp/main.cpp)

target_link_libraries(${APP_NAME} PRIVATE app_support)

# Resolved against the working directory, not the build tree: the app reads
# src/common/config.yaml and src/common/coco_label.txt relative to this, so
# run it from the repository root.
target_compile_definitions(${APP_NAME}
  PRIVATE
    SIMANEAT_APPS_EXAMPLE_SOURCE_DIR="src/cpp"
)

install(TARGETS ${APP_NAME} RUNTIME DESTINATION bin)

message(STATUS "")
message(STATUS "  ${PROJECT_NAME}")
message(STATUS "    NEAT core   : ${NEAT_TARGET}")
message(STATUS "    Build type  : ${CMAKE_BUILD_TYPE}")
message(STATUS "    Binary      : ${APP_NAME}")
message(STATUS "")
