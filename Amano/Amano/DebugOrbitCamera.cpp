#include "DebugOrbitCamera.h"

namespace Amano {

DebugOrbitCamera::DebugOrbitCamera()
	: InputReader()
	, m_width{ 1.0f }
	, m_height{ 1.0f }
	, m_isDragging{ false }
	, m_mousePrevPos()
	, m_cameraOrbitAnglesAndDistance(45.0f, 45.0f, 2.0f)
	, m_cameraPosition()
{
	updateCameraPosition();
}

DebugOrbitCamera::~DebugOrbitCamera() {}


void DebugOrbitCamera::setFrameSize(uint32_t width, uint32_t height) {
	m_width = static_cast<float>(width);
	m_height = static_cast<float>(height);
}

bool DebugOrbitCamera::updateMouse(GLFWwindow* window) {
	// dragging
	double xpos, ypos = 0.0;
	glfwGetCursorPos(window, &xpos, &ypos);

	bool isDragging = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
	glm::vec2 newPos(static_cast<float>(xpos), static_cast<float>(ypos));

	// move!
	if (isDragging && m_isDragging) {
		glm::vec3 moveDir(newPos - m_mousePrevPos, 0.0f);
		// scale the move direction so that it feels the same along x and y axis
		// I chose to scale along x because the window is usually wider than higher
		moveDir *= glm::vec3(m_height / m_width, 1.0f, 1.0f);
		m_cameraOrbitAnglesAndDistance -= moveDir;
		// hardcoded limits
		while (m_cameraOrbitAnglesAndDistance.x > 360.0f)
			m_cameraOrbitAnglesAndDistance.x -= 360.0f;
		while (m_cameraOrbitAnglesAndDistance.x < 0.0f)
			m_cameraOrbitAnglesAndDistance.x += 360.0f;
		while (m_cameraOrbitAnglesAndDistance.y > 180.0f)
			m_cameraOrbitAnglesAndDistance.y -= 180.0f;
		while (m_cameraOrbitAnglesAndDistance.y < 0.0f)
			m_cameraOrbitAnglesAndDistance.y += 180.0f;

		updateCameraPosition();
	}

	// set status and save position if necessary
	m_isDragging = isDragging;
	if (m_isDragging)
		m_mousePrevPos = newPos;

	return true;
}

bool DebugOrbitCamera::updateScroll(double xscroll, double yscroll) {
	// scroll
	m_cameraOrbitAnglesAndDistance.z -= static_cast<float>(yscroll) / 10.0f;
	// hardcoded limits
	if (m_cameraOrbitAnglesAndDistance.z < 0.01f)
		m_cameraOrbitAnglesAndDistance.z = 0.01f;
	updateCameraPosition();

	return true;
}

void DebugOrbitCamera::updateCameraPosition() {
	float tmp = sinf(glm::radians(m_cameraOrbitAnglesAndDistance.y));
	m_cameraPosition.z = m_cameraOrbitAnglesAndDistance.z * cosf(glm::radians(m_cameraOrbitAnglesAndDistance.y));
	m_cameraPosition.x = m_cameraOrbitAnglesAndDistance.z * tmp * cosf(glm::radians(m_cameraOrbitAnglesAndDistance.x));
	m_cameraPosition.y = m_cameraOrbitAnglesAndDistance.z * tmp * sinf(glm::radians(m_cameraOrbitAnglesAndDistance.x));
}
}
