#version 450 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

layout (location = 0) out vec4 out_position;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out uint out_material_index;

uniform uint material_index;

void main()
{
	out_position = vec4(position, 1.0);
	out_normal = vec4(normal, 1.0);
	out_material_index = material_index;
}
