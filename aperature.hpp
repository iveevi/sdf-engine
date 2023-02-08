#pragma once

// Standard headers
#include <tuple>

// GLM headers
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

inline std::tuple <glm::vec3, glm::vec3, glm::vec3>
uvw_frame(const Aperature &aperature, const glm::mat4 &transform)
{
	static constexpr glm::vec4 FORWARD {0.0f, 0.0f, -1.0f, 0.0f};
	static constexpr glm::vec4 UP {0.0f, 1.0f, 0.0f, 0.0f};
		
	glm::vec3 forward = transform * FORWARD;
	glm::vec3 up = transform * UP;

	glm::vec3 eye = glm::vec3(transform[3]);
	glm::vec3 lookat = eye + forward;

	glm::vec3 w = lookat - eye;
	float wlen = glm::length(w);
	glm::vec3 u = glm::normalize(glm::cross(w, up));
	glm::vec3 v = glm::normalize(glm::cross(u, w));

	float vlen = wlen * glm::tan(glm::radians(aperature.m_fov) / 2.0f);
	v *= vlen;

	float ulen = vlen * aperature.m_aspect;
	u *= ulen;

	return {u, v, w};
}
