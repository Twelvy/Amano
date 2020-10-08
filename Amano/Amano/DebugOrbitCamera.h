#pragma once

#include "InputSystem.h"
#include "glm.h"

namespace Amano {

class DebugOrbitCamera : public InputReader
{
public:
	DebugOrbitCamera();
	~DebugOrbitCamera();

	void setFrameSize(uint32_t width, uint32_t height);

	bool updateMouse(GLFWwindow* window) final;
	bool updateScroll(double, double) final;

	glm::vec3 getCameraPosition() const { return m_cameraPosition; }
	glm::vec3 getCameraOrbitAnglesAndDistance() const { return m_cameraOrbitAnglesAndDistance; }

private:
	void updateCameraPosition();

private:
	float m_width;
	float m_height;
	bool m_isDragging;
	glm::vec2 m_mousePrevPos;
	glm::vec3 m_cameraOrbitAnglesAndDistance;
	glm::vec3 m_cameraPosition;
};

}
