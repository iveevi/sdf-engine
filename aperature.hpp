#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

struct Aperature {
	Aperature(float fov = 45.0f, float aspect = 1.0f)
			: m_fov(fov), m_aspect(aspect) {}

	// Perspective projection matrix
	glm::mat4 perspective_matrix() const {
		return glm::perspective(
			glm::radians(m_fov),
			m_aspect, 0.1f, 1000.0f
		);
	}

	// View matrix
	static glm::mat4 view_matrix(const glm::mat4 &transform) {
		static constexpr glm::vec4 FORWARD {0.0f, 0.0f, -1.0f, 0.0f};
		static constexpr glm::vec4 UP {0.0f, 1.0f, 0.0f, 0.0f};

		glm::vec3 position = glm::vec3(transform[3]);
		glm::vec3 forward = transform * FORWARD;
		glm::vec3 up = transform * UP;

		return glm::lookAt(
			position,
			position + forward,
			up
		);
	}
	
	float m_fov;
	float m_aspect;
};
