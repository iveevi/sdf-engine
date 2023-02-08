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

struct Framebuffer {
	unsigned int framebuffer;

	// G-buffers
	unsigned int g_position;
	unsigned int g_normal;
	unsigned int g_material_index;
};

Framebuffer allocate_gl_framebuffer();

// Quad rendering for the final image
void render_final_image()
{
	static unsigned int vao = 0;
	static unsigned int vbo;

	if (vao == 0) {
		constexpr float quad[] = {
			-1.0f,  1.0f, 0.0f,	0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f,	0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f,	1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f,	1.0f, 0.0f,
		};

		// setup plane VAO
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad), &quad, GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

int main()
{
	// TODO: HDR output framebuffer?

	// TODO: materials in storage buffers for easy access in path tracer

	// TODO: for path tracing: G-buffer using position, normal, and material
	// index outputs

	// Initialize GLFW
	GLFWwindow *window = glfw_init();
	if (!window)
		return -1;

	// Load shaders
	// unsigned int vertex_shader = compile_shader("../shaders/basic.vert", GL_VERTEX_SHADER);
	// unsigned int fragment_shader = compile_shader("../shaders/albedo.frag", GL_FRAGMENT_SHADER);

	unsigned int vertex_shader = compile_shader("../shaders/gbuffer.vert", GL_VERTEX_SHADER);
	unsigned int fragment_shader = compile_shader("../shaders/gbuffer.frag", GL_FRAGMENT_SHADER);

	unsigned int quad_vertex_shader = compile_shader("../shaders/quad.vert", GL_VERTEX_SHADER);
	unsigned int quad_fragment_shader = compile_shader("../shaders/quad.frag", GL_FRAGMENT_SHADER);

	unsigned int path_tracer_shader = compile_shader("../shaders/render.glsl", GL_COMPUTE_SHADER);

	// Create shader programs
	unsigned int shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	link_program(shader_program);

	unsigned int quad_shader_program = glCreateProgram();
	glAttachShader(quad_shader_program, quad_vertex_shader);
	glAttachShader(quad_shader_program, quad_fragment_shader);
	link_program(quad_shader_program);

	unsigned int path_tracer_program = glCreateProgram();
	glAttachShader(path_tracer_program, path_tracer_shader);
	link_program(path_tracer_program);

	// Load model and all its buffers
	Model model = load_model("../../models/salle_de_bain/salle_de_bain.obj");

	std::vector <GLBuffers> buffers;
	for (const Mesh &mesh : model.meshes)
		buffers.push_back(allocate_gl_buffers(&mesh));

	// Enable depth testing
	glEnable(GL_DEPTH_TEST);

	// Create the framebuffer
	Framebuffer fb = allocate_gl_framebuffer();

	// Allocate a destination texture for the compute shader
	unsigned int render_target;
	glGenTextures(1, &render_target);
	glBindTexture(GL_TEXTURE_2D, render_target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Unbind framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

		// Bind framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, fb.framebuffer);

		// Clear
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
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
			unsigned int material_index = buffer.source->material_index;
			const Material &material = Material::all[material_index];

			/* bool has_diffuse_texture = material.diffuse_texture.has_value();
			set_vec3(shader_program, "diffuse", material.diffuse);
			set_int(shader_program, "has_diffuse_texure", has_diffuse_texture);

			// Bind diffuse texture
			if (has_diffuse_texture) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, material.diffuse_texture->id);
			} */

			set_uint(shader_program, "material_index", material_index);

			glBindVertexArray(buffer.vao);
			glDrawElements(GL_TRIANGLES, buffer.count, GL_UNSIGNED_INT, 0);
		}

		// Run the compute shader
		glUseProgram(path_tracer_program);

		// Run the shader
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, fb.g_position);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, fb.g_normal);

		// glActiveTexture(GL_TEXTURE3);
		// glBindTexture(GL_TEXTURE_2D, fb.g_material_index);
		
		glBindImageTexture(0, render_target, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
		glBindImageTexture(3, fb.g_material_index, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);

		glDispatchCompute(WIDTH, HEIGHT, 1);

		// Render the final image
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(quad_shader_program);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, render_target);

		render_final_image();

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

Framebuffer allocate_gl_framebuffer()
{
	// Generate G-buffer for custom path tracer
	unsigned int g_buffer;
	glGenFramebuffers(1, &g_buffer);

	// Generate textures for G-buffer
	unsigned int g_position;
	glGenTextures(1, &g_position);
	glBindTexture(GL_TEXTURE_2D, g_position);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, WIDTH, HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	unsigned int g_normal;
	glGenTextures(1, &g_normal);
	glBindTexture(GL_TEXTURE_2D, g_normal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, WIDTH, HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	unsigned int g_material_index;
	glGenTextures(1, &g_material_index);
	glBindTexture(GL_TEXTURE_2D, g_material_index);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, WIDTH, HEIGHT, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Attach textures to G-buffer
	glBindFramebuffer(GL_FRAMEBUFFER, g_buffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_position, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g_normal, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, g_material_index, 0);

	// Set draw buffers
	unsigned int attachments[3] = {
		GL_COLOR_ATTACHMENT0,
		GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2
	};

	glDrawBuffers(3, attachments);

	// Depth buffer
	unsigned int g_depth;
	glGenTextures(1, &g_depth);
	glBindTexture(GL_TEXTURE_2D, g_depth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, WIDTH, HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, g_depth, 0);

	// Check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		fprintf(stderr, "Framebuffer is not complete!\n");
		return {};
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return Framebuffer {
		g_buffer,
		g_position,
		g_normal,
		g_material_index,
	};
}
