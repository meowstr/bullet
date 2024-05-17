cmake_minimum_required( VERSION 3.28 )

project( bullet )

# setup libraries that we store in repo

add_library( glad libs/glad/src/glad.c )
target_include_directories( glad PUBLIC libs/glad/include )

add_library( cglm INTERFACE )
target_include_directories( cglm INTERFACE libs/cglm/include )

add_library( stb INTERFACE )
target_include_directories( stb INTERFACE libs/stb/include )

if ( UNIX )

  # pull libraries from the system

  find_package( PkgConfig REQUIRED )

  pkg_check_modules( GLFW REQUIRED IMPORTED_TARGET glfw3 )
  pkg_check_modules( OPENAL REQUIRED IMPORTED_TARGET openal )

  add_library( glfw ALIAS PkgConfig::GLFW )
  add_library( openal ALIAS PkgConfig::OPENAL )

endif()

if ( WIN32 )

  # pull prebuilt libraries

endif()

add_executable( app 

  # includes
  src/audio.hpp
  src/color.hpp
  src/font.hpp
  src/hardware.hpp
  src/logging.hpp
  src/render.hpp
  src/render_utils.hpp
  src/res.hpp
  src/shape.hpp
  src/state.hpp
  src/utils.hpp
  src/wav.hpp

  # sources
  src/audio.cpp
  src/color.cpp
  src/font.cpp
  src/hsv.cpp
  src/logging.cpp
  src/main.cpp
  src/render.cpp
  src/render_utils.cpp
  src/res.cpp
  src/shape.cpp
  src/state.cpp
  src/utils.cpp
  src/wav.cpp

  # platform dependent
  src/platform/desktop.cpp
)

target_link_libraries( app PRIVATE glad cglm stb glfw openal )
target_include_directories( app PRIVATE src )
target_compile_features( app PRIVATE cxx_std_20 )

add_custom_target( run
    COMMAND app
    DEPENDS app
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)
