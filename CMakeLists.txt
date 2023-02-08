cmake_minimum_required(VERSION 3.0)

# Set standard
set(CMAKE_CXX_STANDARD 17)

project(sdf-engine)

# GLFW
find_package(glfw3 REQUIRED)

add_executable(sdf-engine
	main.cpp
	mesh.cpp
	image.cpp
	glad/src/glad.c
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/tinyexr/deps/miniz/miniz.c

	${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_draw.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_demo.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_widgets.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/imgui_tables.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_glfw.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_opengl3.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/vendor/implot/implot.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/implot/implot_items.cpp
)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/glad/include/
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm/
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/
	${CMAKE_CURRENT_SOURCE_DIR}/vendor/tinyexr/deps/miniz/
)

target_link_libraries(sdf-engine
    glfw
    ${CMAKE_DL_LIBS}
)
