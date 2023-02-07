#version 450 core

layout (location = 0) in vec2 uv;

layout (location = 0) out vec4 fragment;

uniform vec3 diffuse;

uniform int has_diffuse_texure;
layout (binding = 0) uniform sampler2D diffuse_texture;

void main()
{
	if (has_diffuse_texure == 1)
		fragment = texture(diffuse_texture, uv);
	else
		fragment = vec4(diffuse, 1.0);
}
