# Build settings
set(BUILD_TARGET "FastTranslucentShader")

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

configure_file("${CMAKE_CURRENT_LIST_DIR}/settings.h.in"
               "${CMAKE_CURRENT_LIST_DIR}/settings.h" @ONLY)

set(SOURCES main.cpp
            maingui.cpp maingui.h
            openglviewer.cpp openglviewer.h
            arcballcontroller.cpp arcballcontroller.h
            tiny_obj_loader.h settings.h)

set(SHADERS shaders/render.vs shaders/render.fs
            shaders/gbuffers.vs shaders/gbuffers.fs)

include_directories(${spica_INCLUDE_DIRS})
include_directories(${OpenCV_INCLIDE_DIRS})
add_executable(${BUILD_TARGET} ${SOURCES} ${SHADERS})
qt5_use_modules(${BUILD_TARGET} Widgets OpenGL)

source_group("Source Files" FILES ${SOURCES})
source_group("Shader Files" FILES ${SHADERS})

if (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
  set_property(TARGET ${BUILD_TARGET} APPEND PROPERTY
               LINK_FLAGS "/DEBUG /PROFILE")
endif()

target_link_libraries(${BUILD_TARGET} ${QT_LIBRARIES})
target_link_libraries(${BUILD_TARGET} ${OPENGL_LIBRARIES})
target_link_libraries(${BUILD_TARGET} ${Boost_LIBRARIES})
target_link_libraries(${BUILD_TARGET} ${OpenCV_LIBS})
