PROJECT( example )

set(PLATFORM_LIBRARIES
    ${OPENGL_LIBRARY}
    ${GLUT_LIBRARIES}
)

set(SOURCE_FILES "example.cpp")

if(GLEW_SOURCE)
  message(STATUS "found Glew source code. Using it instead of library")
  LIST(APPEND SOURCE_FILES ${GLEW_SOURCE})
  add_definitions(-DGLEW_STATIC)
else()
    message(STATUS "using GLEW library")
    LIST(APPEND PLATFORM_LIBRARIES ${GLEW_LIBRARY})
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${GLEW_INCLUDE_DIR}
    ${GLUT_INCLUDE_DIR}
)

add_executable(example
  ${OpenGLText_SOURCE_DIR}/OpenGLText.h
  ${OpenGLText_SOURCE_DIR}/OpenGLText.cpp
  ${SOURCE_FILES}
)

# Copy the resources where the executable file will be created
_copy_files_to_target( example "Candy Script_48.bin")
_copy_files_to_target( example "Candy Script_48.tga")
_copy_files_to_target( example "baub_16.bin")
_copy_files_to_target( example "baub_16.tga")
_copy_files_to_target( example "Shelley Allegro BT_26.bin")
_copy_files_to_target( example "Shelley Allegro BT_26.tga")

target_link_libraries(example
    ${PLATFORM_LIBRARIES}
)

