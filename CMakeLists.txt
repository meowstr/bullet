cmake_minimum_required( VERSION 3.28 )

project( bullet )

# setup libraries that we store in repo

if ( NOT DEFINED EMSCRIPTEN )

  add_library( glad libs/glad/src/glad.c )
  target_include_directories( glad PUBLIC libs/glad/include )

endif()

add_library( cglm INTERFACE )
target_include_directories( cglm INTERFACE libs/cglm/include )

add_library( stb INTERFACE )
target_include_directories( stb INTERFACE libs/stb/include )

if ( (NOT DEFINED EMSCRIPTEN) AND UNIX )

  # pull libraries from the system

  find_package( PkgConfig REQUIRED )

  pkg_check_modules( GLFW REQUIRED IMPORTED_TARGET glfw3 )
  pkg_check_modules( OPENAL REQUIRED IMPORTED_TARGET openal )

  add_library( glfw ALIAS PkgConfig::GLFW )
  add_library( openal ALIAS PkgConfig::OPENAL )

endif()

if ( (NOT DEFINED EMSCRIPTEN) AND WIN32 )

  # pull prebuilt libraries

  add_library( glfw STATIC IMPORTED )
  set_target_properties(glfw PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/glfw/lib-vc2019/glfw3.lib)
  target_include_directories(glfw INTERFACE libs/glfw/include)

  add_library( openal SHARED IMPORTED )
  #set_target_properties(openal PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/libs/openal/lib/OpenAL32.dll)
  set_target_properties(openal PROPERTIES IMPORTED_IMPLIB ${PROJECT_SOURCE_DIR}/libs/openal/lib/OpenAL32.lib)
  target_include_directories(openal INTERFACE libs/openal/include)

endif()

set( COMMON_SOURCES
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
)

if ( DEFINED EMSCRIPTEN )
  add_executable( app
    ${COMMON_SOURCES}
    # platform dependent
    src/platform/web.cpp
  )

  target_link_libraries( app PRIVATE cglm stb )
  set_target_properties( app PROPERTIES LINK_FLAGS "-s USE_GLFW=3 --shell-file ${PROJECT_SOURCE_DIR}/shell.html" )
  set(CMAKE_EXECUTABLE_SUFFIX ".html")

else()

  if ( WIN32 )
    add_executable( app WIN32 
      ${COMMON_SOURCES}
      # platform dependent
      src/platform/desktop.cpp
    )
  endif()
  
  if ( UNIX )
    add_executable( app
      ${COMMON_SOURCES}
      # platform dependent
      src/platform/desktop.cpp
    )
  endif()

  target_link_libraries( app PRIVATE glad cglm stb glfw openal )

  add_custom_target( run
      COMMAND app
      DEPENDS app
      WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
  )

endif()

target_include_directories( app PRIVATE src )
target_compile_features( app PRIVATE cxx_std_20 )

