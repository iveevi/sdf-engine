#version 450 core

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout (binding = 0, rgba32f) uniform writeonly image2D image;

layout (binding = 1) uniform sampler2D positions;
layout (binding = 2) uniform sampler2D normals;
layout (binding = 3) uniform sampler2D materials;
layout (binding = 4) uniform usampler2D material_indices;

struct Material {
	vec3 diffuse;
	vec3 specular;
	vec3 emission;
	float roughness;
};

Material material_at(int index)
{
	Material material;

	vec4 packed_0 = texelFetch(materials, ivec2(index * 4 + 0, 0), 0);
	vec4 packed_1 = texelFetch(materials, ivec2(index * 4 + 1, 0), 0);
	vec4 packed_2 = texelFetch(materials, ivec2(index * 4 + 2, 0), 0);
	vec4 packed_3 = texelFetch(materials, ivec2(index * 4 + 3, 0), 0);

	material.diffuse = packed_0.xyz;
	material.specular = packed_1.xyz;
	material.emission = packed_2.xyz;
	material.roughness = packed_3.x;

	return material;
}

void main()
{
	// TODO: submesh colorer using material index and color wheel
	ivec2 img_idx = ivec2(gl_GlobalInvocationID.xy);

	unsigned int material_index = texelFetch(material_indices, img_idx, 0).x;
	if (material_index == 0) {
		imageStore(image, img_idx, vec4(0, 1, 1, 0));
		return;
	}

	uvec2 size = imageSize(image);
	vec2 uv = vec2(img_idx)/vec2(size);

	vec3 position = texelFetch(positions, img_idx, 0).xyz;
	vec3 normal = texelFetch(normals, img_idx, 0).xyz;

	Material material = material_at(int(material_index));

	imageStore(image, img_idx, vec4(material.diffuse + material.emission, 0.0));
}
