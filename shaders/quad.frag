#version 450 core

layout (location = 0) in vec2 uv;
out vec4 fragment;

uniform sampler2D texture;

void main()
{
	fragment = texture2D(texture, uv);
}
