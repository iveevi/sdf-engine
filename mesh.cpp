// Standard headers
#include <algorithm>
#include <filesystem>
#include <unordered_map>

// STB headers
#define STB_IMAGE_IMPLEMENTATION

#include <stb/stb_image.h>

// TinyObjLoader headers
#define TINYOBJLOADER_IMPLEMENTATION

#include <tinyobjloader/tiny_obj_loader.h>

// Engine headers
#include "gl.hpp"
#include "mesh.hpp"

namespace tinyobj {

inline bool operator<(const tinyobj::index_t &a, const tinyobj::index_t &b)
{
	return std::tie(a.vertex_index, a.normal_index, a.texcoord_index)
		< std::tie(b.vertex_index, b.normal_index, b.texcoord_index);
}

inline bool operator==(const tinyobj::index_t &a, const tinyobj::index_t &b)
{
	return std::tie(a.vertex_index, a.normal_index, a.texcoord_index)
		== std::tie(b.vertex_index, b.normal_index, b.texcoord_index);
}

}

inline bool operator==(const Vertex &a, const Vertex &b)
{
	return std::tie(a.position, a.normal, a.uv)
		== std::tie(b.position, b.normal, b.uv);
}

namespace std {

template <>
struct hash <tinyobj::index_t>
{
	size_t operator()(const tinyobj::index_t &k) const
	{
		return ((hash<int>()(k.vertex_index)
			^ (hash<int>()(k.normal_index) << 1)) >> 1)
			^ (hash<int>()(k.texcoord_index) << 1);
	}
};

template <>
struct hash <glm::vec3> {
	size_t operator()(const glm::vec3 &v) const
	{
		return ((hash <float>()(v.x)
			^ (hash <float>()(v.y) << 1)) >> 1)
			^ (hash <float>()(v.z) << 1);
	}
};

template <>
struct hash <glm::vec2> {
	size_t operator()(const glm::vec2 &v) const
	{
		return ((hash <float>()(v.x)
			^ (hash <float>()(v.y) << 1)) >> 1);
	}
};

template <>
struct hash <Vertex> {
	size_t operator()(Vertex const &vertex) const
	{
		return ((hash <glm::vec3>()(vertex.position)
			^ (hash <glm::vec3>()(vertex.normal) << 1)) >> 1)
			^ (hash <glm::vec2>()(vertex.uv) << 1);
	}
};

}

// All materials
std::vector <Material> Material::all {
	// Pad with a material to simplify things...
	Material {}
};

// Load a model from a file
Model load_model(const std::string &path)
{
	// Check if the file exists
	if (!std::filesystem::exists(path)) {
		fprintf(stderr, "Could not find model at path provided: %s", path.c_str());
		return {};
	}

	// Loader configuration
	tinyobj::ObjReaderConfig reader_config;
	reader_config.mtl_search_path = std::filesystem::path(path).parent_path().string();

	// Loader
	tinyobj::ObjReader reader;

	// Load the mesh
	if (!reader.ParseFromFile(path, reader_config)) {
		fprintf(stderr, "Failed to load model: %s", path.c_str());
		return {};
	}

	// Warnings
	if (!reader.Warning().empty())
		fprintf(stderr, "Warning: %s", reader.Warning().c_str());

	// Get the mesh properties
	auto &attrib = reader.GetAttrib();
	auto &shapes = reader.GetShapes();
	auto &materials = reader.GetMaterials();

	// Load submeshes
	std::vector <Mesh> meshes;
	std::vector <int> emissive_meshes;

	for (int i = 0; i < shapes.size(); i++) {
		auto &mesh = shapes[i].mesh;

		std::vector <Vertex> vertices;
		std::vector <uint32_t> indices;

		std::unordered_map <Vertex, uint32_t> unique_vertices;
		std::unordered_map <tinyobj::index_t, uint32_t> index_map;

		int offset = 0;
		for (int f = 0; f < mesh.num_face_vertices.size(); f++) {
			// Get the number of vertices in the face
			int fv = mesh.num_face_vertices[f];

			// Loop over vertices in the face
			for (int v = 0; v < fv; v++) {
				// Get the vertex index
				tinyobj::index_t index = mesh.indices[offset + v];

				if (index_map.count(index) > 0) {
					indices.push_back(index_map[index]);
					continue;
				}

				Vertex vertex;

				vertex.position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				if (index.normal_index >= 0) {
					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2]
					};
				} else {
					// Compute geometric normal with
					// respect to this face

					// TODO: method
					int pindex = (v - 1 + fv) % fv;
					int nindex = (v + 1) % fv;

					tinyobj::index_t p = mesh.indices[offset + pindex];
					tinyobj::index_t n = mesh.indices[offset + nindex];

					glm::vec3 vn = {
						attrib.vertices[3 * p.vertex_index + 0],
						attrib.vertices[3 * p.vertex_index + 1],
						attrib.vertices[3 * p.vertex_index + 2]
					};

					glm::vec3 vp = {
						attrib.vertices[3 * n.vertex_index + 0],
						attrib.vertices[3 * n.vertex_index + 1],
						attrib.vertices[3 * n.vertex_index + 2]
					};

					glm::vec3 e1 = vp - vertex.position;
					glm::vec3 e2 = vn - vertex.position;

					vertex.normal = glm::normalize(glm::cross(e1, e2));
				}

				if (index.texcoord_index >= 0) {
					vertex.uv = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						1 - attrib.texcoords[2 * index.texcoord_index + 1]
					};
				} else {
					vertex.uv = {0.0f, 0.0f};
				}

				// Add the vertex to the list
				// vertices.push_back(vertex);
				// indices.push_back(vertices.size() - 1);

				// Add the vertex
				uint32_t id;
				if (unique_vertices.count(vertex) > 0) {
					id = unique_vertices[vertex];
				} else {
					id = vertices.size();
					unique_vertices[vertex] = id;
					vertices.push_back(vertex);
				}

				index_map[index] = id;
				indices.push_back(id);
			}

			// Update the offset
			offset += fv;

			// If last face, or material changes
			// push back the submesh
			if (f == mesh.num_face_vertices.size() - 1 ||
					mesh.material_ids[f] != mesh.material_ids[f + 1]) {
				Material material;

				// TODO: method
				if (mesh.material_ids[f] < materials.size()) {
					tinyobj::material_t m = materials[mesh.material_ids[f]];
					material.diffuse = {m.diffuse[0], m.diffuse[1], m.diffuse[2]};
					material.specular = {m.specular[0], m.specular[1], m.specular[2]};
					material.emission = {m.emission[0], m.emission[1], m.emission[2]};

					// Surface properties
					// mat.shininess = m.shininess;
					material.roughness = sqrt(2.0f / (m.shininess + 2.0f));
					// mat.roughness = glm::clamp(1.0f - mat.shininess/1000.0f, 1e-3f, 0.999f);
					// mat.refraction = m.ior;

					// Albedo texture
					if (!m.diffuse_texname.empty()) {
						std::replace(m.diffuse_texname.begin(), m.diffuse_texname.end(), '\\', '/');
						printf("Diffuse texture: %s\n", m.diffuse_texname.c_str());

						// Search for file in the search path
						std::filesystem::path p(reader_config.mtl_search_path);
						p /= m.diffuse_texname;

						printf("Resolved path: %s\n", p.c_str());

						// If the file exists, use it
						if (std::filesystem::exists(p)) {
							printf("File exists\n");
							material.diffuse_texture = allocate_gl_texture(p.c_str());
						}
					}

					/* Normal texture
					if (!m.normal_texname.empty()) {
						mat.normal_texture = m.normal_texname;
						mat.normal_texture = common::resolve_path(
							m.normal_texname, {reader_config.mtl_search_path}
						);
					}

					// Specular texture
					if (!m.specular_texname.empty()) {
						mat.specular_texture = m.specular_texname;
						mat.specular_texture = common::resolve_path(
							m.specular_texname, {reader_config.mtl_search_path}
						);
					}

					// Emission texture
					if (!m.emissive_texname.empty()) {
						mat.emission_texture = m.emissive_texname;
						mat.emission_texture = common::resolve_path(
							m.emissive_texname, {reader_config.mtl_search_path}
						);

						mat.type = eEmissive;
					} */
				}

				// Add submesh
				int material_index = Material::all.size();
				Material::all.push_back(material);

				if (glm::length(material.emission)) {
					emissive_meshes.push_back(meshes.size());
					printf("Emission material\n");
				}
				
				meshes.push_back(Mesh {vertices, indices, material_index});

				// Clear the vertices and indices
				unique_vertices.clear();
				index_map.clear();
				vertices.clear();
				indices.clear();
			}
		}
	}

	return Model {meshes, emissive_meshes};
}

GLBuffers allocate_gl_buffers(const Mesh *mesh)
{
	GLBuffers buffers;

	// Generate VAO, VBO, and EBO
	glGenVertexArrays(1, &buffers.vao);
	glGenBuffers(1, &buffers.vbo);
	glGenBuffers(1, &buffers.ebo);

	// Bind VAO
	glBindVertexArray(buffers.vao);

	// Bind VBO
	glBindBuffer(GL_ARRAY_BUFFER, buffers.vbo);
	glBufferData(GL_ARRAY_BUFFER,
		mesh->vertices.size() * sizeof(Vertex),
		mesh->vertices.data(),
		GL_STATIC_DRAW
	);

	// Bind EBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		mesh->indices.size() * sizeof(unsigned int),
		mesh->indices.data(),
		GL_STATIC_DRAW
	);

	// Vertex attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) 0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, normal));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *) offsetof(Vertex, uv));

	buffers.count = mesh->indices.size();
	buffers.source = mesh;

	return buffers;
}

// Cache of loaded textures
std::map <std::string, GLTexture> GLTexture::all;

GLTexture allocate_gl_texture(const std::string &path)
{
	// Check if the texture has already been loaded
	if (GLTexture::all.find(path) != GLTexture::all.end())
		return GLTexture::all[path];

	// Otherwise, load the texture
	GLTexture texture;

	// Load with stb_image
	int width, height, channels;
	unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 0);
	if (!data) {
		fprintf(stderr, "Failed to load texture: %s\n", path.c_str());
		texture.id = 0;
		return texture;
	}

	// Create texture
	glGenTextures(1, &texture.id);
	glBindTexture(GL_TEXTURE_2D, texture.id);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Set texture data
	if (channels == 3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	else if (channels == 4)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	else
		fprintf(stderr, "Unsupported number of channels: %d\n", channels);

	// Generate mipmaps
	glGenerateMipmap(GL_TEXTURE_2D);

	// Free image data
	stbi_image_free(data);

	// Transfer other properties
	texture.path = path;

	// Cache the texture
	GLTexture::all[path] = texture;

	return texture;
}
