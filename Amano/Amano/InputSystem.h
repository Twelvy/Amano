#pragma once

#include "glfw.h"

#include <vector>

namespace Amano {

// abstract class that can consume inputs
class InputReader {
public:
	virtual bool updateMouse(GLFWwindow* window) = 0;
	virtual bool updateScroll(double xscroll, double yscroll) = 0;
};

// class redirecting the input to the services consuming them
class InputSystem
{
public:
	InputSystem();
	~InputSystem();

	void update(GLFWwindow* window);
	void updateScroll(double xscroll, double yscroll);  // scroll insn't a state, so it is event based
	void registerReader(InputReader* reader);

private:
	std::vector<InputReader*> m_readers;
};

}
