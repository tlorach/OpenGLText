cmake_minimum_required(VERSION 2.8)

PROJECT( OpenGLText )

# Specify the list of directories to search for cmake modules.
set(CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

set( CMAKE_PROGRAM_PATH "$ENV{PATH}")

if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  set (ARCH "x64" CACHE STRING "CPU Architecture")
else ()
  set (ARCH "x86" CACHE STRING "CPU Architecture")
endif()

set(EXECUTABLE_OUTPUT_PATH
    ${PROJECT_BINARY_DIR}/bin
    CACHE PATH
    "Directory where executables will be stored"
)

if(UNIX) 
  set(OS "linux")
  add_definitions(-DLINUX)
else(UNIX)
  if(APPLE)
  else(APPLE)
    if(WIN32)
      set(OS "win")
      add_definitions(-DNOMINMAX)
    endif(WIN32)
  endif(APPLE)
endif(UNIX)

# Macro for adding files close to the executable
macro(_copy_files_to_target target thefiles)
    if(WIN32)
        foreach (FFF ${thefiles} )
			add_custom_command(
			    TARGET ${target} POST_BUILD
			    COMMAND ${CMAKE_COMMAND} -E copy_if_different
			        "${PROJECT_SOURCE_DIR}/${FFF}"
			        $<TARGET_FILE_DIR:${target}>
			        VERBATIM
			)
        endforeach()
    endif()

endmacro()


find_package(OpenGL)
# ===============> GLUT
find_package(GLUT)
if(NOT GLUT_FOUND)
  message(WARNING "Try to set GLUT_LOCATION")
endif()
# ===============> GLEW
if (NOT APPLE)
  find_package(GLEW REQUIRED)
endif()

add_subdirectory(example)
