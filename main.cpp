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
	// TODO: materials in storage buffers for easy access in path tracer

	// TODO: for path tracing: G-buffer using position, normal, and material
	// index outputs

	// Initialize GLFW
	GLFWwindow *window = glfw_init();
	if (!window)
		return -1;

	// Load shaders
	unsigned int vertex_shader = compile_shader("../shaders/basic.vert", GL_VERTEX_SHADER);
	unsigned int fragment_shader = compile_shader("../shaders/albedo.frag", GL_FRAGMENT_SHADER);

	// Create shader program
	unsigned int shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);

	link_program(shader_program);

	// Load model and all its buffers
	Model model = load_model("../../models/salle_de_bain/salle_de_bain.obj");

	std::vector <GLBuffers> buffers;
	for (const Mesh &mesh : model.meshes)
		buffers.push_back(allocate_gl_buffers(&mesh));

	// Enable depth testing
	glEnable(GL_DEPTH_TEST);

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
			set_vec3(shader_program, "diffuse", buffer.source->material.diffuse);

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

static bool dragging = false;

static void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
	static double last_x = WIDTH/2.0;
	static double last_y = HEIGHT/2.0;

	static float sensitivity = 0.1f;

	static bool first_mouse = true;

	static float yaw = 0.0f;
	static float pitch = 0.0f;

	if (first_mouse) {
		last_x = xpos;
		last_y = ypos;
		first_mouse = false;
	}

	double xoffset = last_x - xpos;
	double yoffset = last_y - ypos;

	last_x = xpos;
	last_y = ypos;

	xoffset *= sensitivity;
	yoffset *= sensitivity;

	// Only drag when left mouse button is pressed
	if (dragging) {
		yaw += xoffset;
		pitch += yoffset;

		// Clamp pitch
		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;

		// Set camera transform yaw and pitch
		glm::mat4 &transform = camera.transform;

		// First decompose the transform matrix
		glm::vec3 translation;
		glm::vec3 scale;
		glm::quat rotation;
		glm::vec3 skew;
		glm::vec4 perspective;

		glm::decompose(transform, scale, rotation, translation, skew, perspective);

		// Then set the rotation
		rotation = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), 0.0f));

		// Finally recompose the transform matrix
		transform = glm::translate(glm::mat4 {1.0f}, translation);
		transform = transform * glm::mat4_cast(rotation);
		transform = glm::scale(transform, scale);
	}
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		dragging = true;
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
		dragging = false;
}

GLFWwindow *glfw_init()
{
	// Basic window
	GLFWwindow *window = nullptr;
	if (!glfwInit())
		return nullptr;

	window = glfwCreateWindow(WIDTH, HEIGHT, "SDF Engine", NULL, NULL);

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

	// Set up callbacks
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	// glfwSetKeyCallback(window, keyboard_callback);

	const GLubyte* renderer = glGetString(GL_RENDERER);
	printf("Renderer: %s\n", renderer);

	return window;
}
