#include <iostream>

#include "aperature.hpp"
#include "mesh.hpp"
#include "shader.hpp"

constexpr int WIDTH = 1000;
constexpr int HEIGHT = 1000;

GLFWwindow *glfw_init();

static struct {
	glm::mat4 transform {1.0f};
	Aperature aperature {};
} camera;

int main()
{
	// Initialize GLFW
	GLFWwindow *window = glfw_init();
	if (!window)
		return -1;

	// Load shaders
	unsigned int vertex_shader = compile_shader("../shaders/basic.vert", GL_VERTEX_SHADER);
	unsigned int fragment_shader = compile_shader("../shaders/basic.frag", GL_FRAGMENT_SHADER);

	// Create shader program
	unsigned int shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);

	link_program(shader_program);

	// Load model and all its buffers
	Model model = load_model("../../models/salle_de_bain/salle_de_bain.obj");

	std::vector <GLBuffers> buffers;
	for (const Mesh &mesh : model.meshes)
		buffers.push_back(allocate_gl_buffers(mesh));

	// Enable depth testing
	glEnable(GL_DEPTH_TEST);
	// glDepthFunc(GL_ALWAYS);

	// Main loop
	while (!glfwWindowShouldClose(window)) {
		constexpr float speed = 0.25f;

		// Handle camera movement
		glm::vec3 diff {0.0};
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			diff.z -= speed;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			diff.x -= speed;
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			diff.y += speed;

		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			diff.x += speed;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			diff.z += speed;
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			diff.y -= speed;

		camera.transform = glm::translate(camera.transform, diff);

		// Clear
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Load shader and set uniforms
		glUseProgram(shader_program);

		glm::mat4 view = camera.aperature.view_matrix(camera.transform);
		// TODO: pass extent to this method
		glm::mat4 projection = camera.aperature.perspective_matrix();

		set_mat4(shader_program, "model", glm::mat4 {1.0f});
		set_mat4(shader_program, "view", view);
		set_mat4(shader_program, "projection", projection);

		// Draw
		// TODO: use common VAO...
		glBindVertexArray(buffers[0].vao);
		for (const GLBuffers &buffer : buffers) {
			glBindVertexArray(buffer.vao);
			glDrawElements(GL_TRIANGLES, buffer.count, GL_UNSIGNED_INT, 0);
		}

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Clean up
	glfwTerminate();

	return 0;
}

GLFWwindow *glfw_init()
{
	// Basic window
	GLFWwindow *window = nullptr;
	if (!glfwInit())
		return nullptr;

	window = glfwCreateWindow(WIDTH, HEIGHT, "Tranquil", NULL, NULL);

	// Check if window was created
	if (!window) {
		glfwTerminate();
		return nullptr;
	}

	// Make the window's context current
	glfwMakeContextCurrent(window);

	// Load OpenGL functions using GLAD
	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
		fprintf(stderr, "Failed to initialize OpenGL context\n");
		return nullptr;
	}

	/* Set up callbacks
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetKeyCallback(window, keyboard_callback); */

	const GLubyte* renderer = glGetString(GL_RENDERER);
	printf("Renderer: %s\n", renderer);

	return window;
}
