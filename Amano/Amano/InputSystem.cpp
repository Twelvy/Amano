#include "InputSystem.h"

namespace Amano {

InputSystem::InputSystem()
	: m_readers()
{
}

InputSystem::~InputSystem() {}

void InputSystem::update(GLFWwindow* window) {
	for (auto reader : m_readers) {
		if (reader->updateMouse(window))
			break;
	}
}

void InputSystem::updateScroll(double xscroll, double yscroll) {
	for (auto reader : m_readers) {
		if (reader->updateScroll(xscroll, yscroll))
			break;
	}
}

void InputSystem::registerReader(InputReader* reader) {
	m_readers.push_back(reader);
}

}
