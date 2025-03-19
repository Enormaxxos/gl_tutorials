# Set the minimum CMake version
cmake_minimum_required(VERSION 3.10)

project(DepthOfField)

add_executable(DepthOfField
	main.cpp
	renderer.hpp
	scene_definition.hpp
)
target_sources(DepthOfField PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}/../../glad/src/glad.c
)
target_link_libraries(DepthOfField utils glm::glm glfw OpenGL::GL)
target_include_directories(DepthOfField PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../../glad/include
	${CMAKE_CURRENT_SOURCE_DIR}/../../utils
	${CMAKE_CURRENT_SOURCE_DIR}/../..
	${CMAKE_CURRENT_SOURCE_DIR}
)
