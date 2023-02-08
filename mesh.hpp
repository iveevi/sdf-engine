#pragma once

// Standard headers
#include <map>
#include <optional>
#include <string>
#include <vector>

// GLM headers
#include <glm/glm.hpp>

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;

	// glm::vec3 tangent;
	// glm::vec3 bitangent;
};

struct GLTexture {
	std::string path;
	unsigned int id;

	static std::map <std::string, GLTexture> all;
};

struct Material {
	glm::vec3 diffuse;
	glm::vec3 specular;
	glm::vec3 emission;
	float roughness;

	std::optional <GLTexture> diffuse_texture = {};

	static std::vector <Material> all;
};

struct Mesh {
	std::vector <Vertex> vertices;
	std::vector <uint32_t> indices;
	int material_index;
	// Material material;
};

struct Model {
	std::vector <Mesh> meshes;
	std::vector <int> emissive_meshes;
};

struct GLBuffers {
	uint32_t vao;
	uint32_t vbo;
	uint32_t ebo;
	uint32_t count;

	const Mesh *source = nullptr;
};

Model load_model(const std::string &);
GLBuffers allocate_gl_buffers(const Mesh *);
GLTexture allocate_gl_texture(const std::string &);
