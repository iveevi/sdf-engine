#version 330 core

out vec4 fragment;

uniform vec3 diffuse;

void main()
{             
	fragment = vec4(diffuse, 1.0);
}
