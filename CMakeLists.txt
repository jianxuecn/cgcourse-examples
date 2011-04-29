cmake_minimum_required(VERSION 2.6)
if(COMMAND CMAKE_POLICY)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND CMAKE_POLICY)

project(cg_examples)

set(CMAKE_MODULE_PATH "${cg_examples_SOURCE_DIR}/cmake")

set(CMAKE_INSTALL_PREFIX "${cg_examples_BINARY_DIR}/dist" CACHE PATH "cg_examples install prefix" FORCE)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(FreeImage QUIET)
find_package(GLEW QUIET)
find_package(MITK QUIET)

#if(NOT GLUT_FOUND)
#  message(FATAL_ERROR "${PROJECT_NAME} requires GLUT.")
#endif(NOT GLUT_FOUND)

include_directories(
  "include"
)

add_custom_target(copy_data ${CMAKE_COMMAND} -E copy_directory ${cg_examples_SOURCE_DIR}/data ${cg_examples_BINARY_DIR}/data)

install(DIRECTORY data/ DESTINATION data PATTERN ".svn" EXCLUDE)
install(DIRECTORY src/shaders/ DESTINATION shaders PATTERN ".svn" EXCLUDE)

function(add_example EXAMPLE_NAME)
  set(LIBRARIES ${GLUT_glut_LIBRARY} ${OPENGL_LIBRARIES})
  if(ARGV1)
    set(ADDITIONAL_LIBRARYS ${ARGV})
	list(REMOVE_AT ADDITIONAL_LIBRARYS 0)
	list(APPEND LIBRARIES ${ADDITIONAL_LIBRARYS})
  endif(ARGV1)
  add_executable(${EXAMPLE_NAME} src/${EXAMPLE_NAME}.cpp)
  target_link_libraries(${EXAMPLE_NAME} ${LIBRARIES})
  install(TARGETS ${EXAMPLE_NAME} RUNTIME DESTINATION . CONFIGURATIONS Release)  
endfunction(add_example)

add_example(sierpinski)
add_example(picksquare)
add_example(transform)
add_example(sphere)
add_example(curves)
add_example(nurbscurve)
add_example(surface)
add_example(gravity)
add_example(gravity2)
add_example(mandelsimp)

if(FREEIMAGE_FOUND)
  add_example(texture2d ${FREEIMAGE_LIBRARIES})
  add_example(mirror ${FREEIMAGE_LIBRARIES})
endif(FREEIMAGE_FOUND)

if(MITK_FOUND)
  include_directories(${MITK_INCLUDE_DIR})
  add_example(spheremap ${MITK_LIBRARIES})
  add_example(dyncubemap ${MITK_LIBRARIES})
endif(MITK_FOUND)

if(GLEW_FOUND)
  configure_file(src/shaders/phong.vert ${cg_examples_BINARY_DIR}/shaders/phong.vert COPYONLY)
  configure_file(src/shaders/phong.frag ${cg_examples_BINARY_DIR}/shaders/phong.frag COPYONLY)
  add_example(phongshader ${GLEW_GLEW_LIBRARY})
endif(GLEW_FOUND)

