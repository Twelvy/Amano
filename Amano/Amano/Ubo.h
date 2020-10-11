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

}
