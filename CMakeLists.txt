cmake_minimum_required(VERSION 3.0.0)
project(FastTranslucentShader)

# -----------------------------------------------------------------------------
# Required packages
# -----------------------------------------------------------------------------
if (WIN32)
  set(QT5_ROOT "QT5-NOT_FOUND" CACHE PATH "")
  if (NOT EXISTS "${QT5_ROOT}/lib/cmake")
    message(FATAL_ERROR "[ERROR] Qt5 not found!!")
  endif()
  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${QT5_ROOT})
endif()

find_package(OpenCV REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5OpenGL REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(OpenGL REQUIRED)

# -----------------------------------------------------------------------------
# Process source directory
# -----------------------------------------------------------------------------
add_subdirectory(sources)
