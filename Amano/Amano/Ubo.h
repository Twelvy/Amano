#pragma once

#include "glm.h"

namespace Amano {

// Uniform buffer for gbuffer vertex shader 
struct PerFrameUniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

// Uniform buffer for raygen shader
struct RayParams {
	glm::mat4 viewInverse;
	glm::mat4 projInverse;
	glm::vec3 rayOrigin;
};

struct LightInformation {
	glm::vec3 lightPosition;
};

struct MaterialInformation {
	glm::vec3 baseColor = glm::vec3(1.0f);
	float reflectance = 1.0f;
	float metalness = 0.1f;
	float roughness = 0.8f;
};

struct DebugInformation {
	float diffuseWeight = 1.0f;
	float specularWeight = 1.0f;
};

}
