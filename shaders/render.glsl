#version 450 core

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout (binding = 0, rgba32f) uniform writeonly image2D image;

layout (binding = 1) uniform sampler2D positions;
layout (binding = 2) uniform sampler2D normals;
layout (binding = 3) uniform sampler2D materials;
layout (binding = 4) uniform usampler2D material_indices;
layout (binding = 5) uniform sampler2D environment;

const float M_PI = 3.1415926535897932384626433832795;

uniform struct {
	vec3 position;
	vec3 axis_u;
	vec3 axis_v;
	vec3 axis_w;
} camera;

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

vec4 tonemap(vec4 color)
{
	vec4 a = color * (2.51f * color + 0.03f);
	vec4 b = color * (2.43f * color + 0.59f) + 0.14f;
	return clamp(a/b, 0.0f, 1.0f);
}

vec2 dir_to_uv(vec3 dir)
{
	float theta = atan(dir.z, dir.x);
	float phi = acos(dir.y);

	float u = theta / (2 * M_PI) + 0.5;
	float v = phi / M_PI;

	return vec2(u, v);
}

void main()
{
	// TODO: submesh colorer using material index and color wheel
	ivec2 img_idx = ivec2(gl_GlobalInvocationID.xy);
	uvec2 size = imageSize(image);
	vec2 uv = vec2(img_idx)/vec2(size);

	unsigned int material_index = texelFetch(material_indices, img_idx, 0).x;
	if (material_index == 0) {
		// Generate camera ray
		vec2 d = 2 * (vec2(img_idx) - vec2(0.5))
			/ vec2(size) - vec2(1.0);

		vec3 dir = normalize(
			camera.axis_u * d.x
			+ camera.axis_v * d.y
			+ camera.axis_w
		);

		// Convert direction to UV coordinates
		vec2 uv = dir_to_uv(dir);
		vec4 env_color = texture(environment, uv);
		// TODO: use texel fetch to make this faster?
		imageStore(image, img_idx, tonemap(env_color));
		return;
	}

	vec3 position = texelFetch(positions, img_idx, 0).xyz;
	vec3 normal = texelFetch(normals, img_idx, 0).xyz;

	Material material = material_at(int(material_index));

	normal = normalize(normal);
	
	vec3 V = normalize(position - camera.position);
	vec3 R = reflect(V, normal);

	vec2 env_uv = dir_to_uv(R);
	vec4 env_color = texture(environment, env_uv);

	vec3 color = material.diffuse
		+ material.emission
		+ env_color.xyz;

	vec4 color4 = vec4(color, 1.0);

	imageStore(image, img_idx, tonemap(color4));
}
