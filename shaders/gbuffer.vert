#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

layout (location = 0) out vec3 out_position;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec2 out_uv;

void main()
{
	vec4 model_position = model * vec4(position, 1.0f);
	gl_Position = projection * view * model_position;

	out_position = model_position.xyz;
	// TODO: pass the tbh matrix
	out_normal = normal;
	out_uv = uv;
}
