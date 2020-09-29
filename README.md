# Amano Engine
This isn't an engine yet. It is a sample rendering project for now.

# Build
The project is using C++17 features.

## Dependencies
The project depends on imgui, glm, stb, tinyobjloader. Those are automatically downloaded when cloning the repository.

You need the VulkanSdk available at https://vulkan.lunarg.com/. The path to it is hardcoded in the solution and the script. Those need manual editing.

Finally you need to download GLFW 3.3.2 available at https://www.glfw.org/. Once downloaded, put it in `External/glfw`.

The Visual Studio 2019 solution is committed. I should replace it by a more flexible system.

## Running
Compile the shaders before running the project. Just execute the script `scripts/compile_shader.bat`.