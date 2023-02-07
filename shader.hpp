#pragma once

#include <fstream>
#include <sstream>
#include <string>

#include <glm/gtc/type_ptr.hpp>

#include "gl.hpp"

inline std::string read_glsl(const std::string &path)
{
	// Open file
	std::ifstream file(path);
	if (!file.is_open()) {
		printf("Failed to open file: %s\n", path.c_str());
		return "";
	}

	// Read lines
	std::string source;

	std::string line;
	while (std::getline(file, line)) {
		// Tokenize line
		std::stringstream ss(line);
		std::string token;

		ss >> token;
		if (token == "#include") {
			ss >> token;

			// check that token is of the format "<file>"
			if (token.size() < 2 || token[0] != '<' || token[token.size() - 1] != '>') {
				printf("Invalid include directive: %s\n", line.c_str());
				return "";
			}

			// Get file name
			std::string file_name = token.substr(1, token.size() - 2);

			// Get file path, which is relative
			std::string file_path = path.substr(0, path.find_last_of('/') + 1);

			// Read file
			std::string include_source = read_glsl(file_path + file_name);

			// Append include source
			source += include_source;
		} else {
			source += line + "\n";
		}
	}

	return source;
}

int compile_shader(const char *path, unsigned int type)
{
	std::string glsl_source = read_glsl(path);
	const char *source = glsl_source.c_str();

	// Write the source to a temporary file
	system("mkdir -p tmp");

	// Extract the filename from the path
	std::string filename = path;
	filename = filename.substr(filename.find_last_of("/") + 1);

	std::ofstream tmp("tmp/out_" + filename);
	tmp << source;
	tmp.close();

	if (!source) {
		printf("Failed to read shader source\n");
		throw std::runtime_error("Failed to read shader source");
	}

	unsigned int shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	// Check for errors
	int success;
	char info_log[512];

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, info_log);
		printf("Failed to compile shader (%s):\n%s\n", path, info_log);
		throw std::runtime_error("Failed to compile shader");
	}

	printf("Successfully compiled shader %s\n", path);
	return shader;
}

int link_program(unsigned int program)
{
	int success;
	char info_log[512];

	glLinkProgram(program);

	// Check if program linked successfully
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success) {
		glGetProgramInfoLog(program, 512, NULL, info_log);
		printf("Failed to link program: %s\n", info_log);
		return 0;
	}

	return 1;
}

void set_int(unsigned int program, const char *name, int value)
{
	glUseProgram(program);
	int i = glGetUniformLocation(program, name);
	glUniform1i(i, value);
}

void set_float(unsigned int program, const char *name, float value)
{
	glUseProgram(program);
	int i = glGetUniformLocation(program, name);
	glUniform1f(i, value);
}

void set_vec2(unsigned int program, const char *name, const glm::vec2 &vec)
{
	glUseProgram(program);
	int i = glGetUniformLocation(program, name);
	glUniform2fv(i, 1, glm::value_ptr(vec));
}

void set_vec3(unsigned int program, const char *name, const glm::vec3 &vec)
{
	glUseProgram(program);
	int i = glGetUniformLocation(program, name);
	glUniform3fv(i, 1, glm::value_ptr(vec));
}

void set_mat4(unsigned int program, const char *name, const glm::mat4 &mat)
{
	glUseProgram(program);
	int i = glGetUniformLocation(program, name);
	glUniformMatrix4fv(i, 1, GL_FALSE, glm::value_ptr(mat));
}
