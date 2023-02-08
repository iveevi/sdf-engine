#version 450 core

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout (binding = 0, rgba32f) uniform writeonly image2D image;

layout (binding = 1) uniform sampler2D positions;
layout (binding = 2) uniform sampler2D normals;

layout (binding = 3, r32ui) uniform readonly uimage2D material_indices;

void main()
{
	// TODO: submesh colorer using material index and color wheel
	ivec2 img_idx = ivec2(gl_GlobalInvocationID.xy);

	// int material_index = int(texelFetch(material_indices, img_idx, 0).x);
	unsigned int material_index = imageLoad(material_indices, img_idx).x;
	if (material_index == 0) {
		imageStore(image, img_idx, vec4(0, 1, 1, 0));
		return;
	}

	uvec2 size = imageSize(image);
	vec2 uv = vec2(img_idx)/vec2(size);

	vec3 position = texelFetch(positions, img_idx, 0).xyz;
	vec3 normal = texelFetch(normals, img_idx, 0).xyz;

	imageStore(image, img_idx, vec4(normal, 0.0));
}
