#include <iostream>
#include <cassert>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <array>

#include "ogl_resource.hpp"
#include "error_handling.hpp"
#include "window.hpp"
#include "shader.hpp"


#include "scene_definition.hpp"
#include "renderer.hpp"

#include "ogl_geometry_factory.hpp"
#include "ogl_material_factory.hpp"

#include <glm/gtx/string_cast.hpp>

void toggle(const std::string &aToggleName, bool &aToggleValue) {

	aToggleValue = !aToggleValue;
	std::cout << aToggleName << ": " << (aToggleValue ? "ON\n" : "OFF\n");
}

struct Config {
	int currentSceneIdx = 0;
	bool showSolid = true;
	bool showWireframe = false;
	bool useZOffset = false;
};

int main() {
	// Initialize GLFW
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	try {
		auto window = Window();
		MouseTracking mouseTracking;
		Config config;
		Camera camera(window.aspectRatio(), 60);
		camera.setPosition(glm::vec3(-6.45035f, 15.8327f, 48.0387f));
		camera.setRotation(glm::quat(0.938374f, -0.13043f, -0.109222f, -0.300853f));
		SpotLight light;
		light.setPosition(glm::vec3(25.0f, 40.0f, 30.0f));
		light.lookAt(glm::vec3());

		float dofFocusStep = 0.05f;

		float dofFocusDistance = 1.0f;
		float dofFocusRange = 1.0f;

		bool dofTesting = true;

		window.onCheckInput([&camera, &mouseTracking](GLFWwindow *aWin) {
				mouseTracking.update(aWin);
				if (glfwGetMouseButton(aWin, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
					camera.orbit(-0.4f * mouseTracking.offset(), glm::vec3());
				}
			});
		window.setKeyCallback([&config, &camera, &dofFocusDistance, &dofFocusRange, dofFocusStep, &dofTesting](GLFWwindow *aWin, int key, int scancode, int action, int mods) {
				if (action == GLFW_PRESS) {
					switch (key) {
					case GLFW_KEY_ENTER:
						camera.setPosition(glm::vec3(-6.45035f, 15.8327f, 48.0387f));
						camera.setRotation(glm::quat(0.938374f, -0.13043f, -0.109222f, -0.300853f));

						dofFocusDistance = 1.0f;
						dofFocusRange = 1.0f;
						break;
					case GLFW_KEY_UP:
						dofFocusDistance = std::max(dofFocusDistance - dofFocusStep, 0.0f);
						std::cout << "Distance: " << dofFocusDistance << std::endl;
						break;
						case GLFW_KEY_DOWN:
						dofFocusDistance += dofFocusStep;
						std::cout << "Distance: " << dofFocusDistance << std::endl;
						break;
						case GLFW_KEY_RIGHT:
						dofFocusRange = std::max(dofFocusRange - dofFocusStep, 0.0f);
						std::cout << "Range: " << dofFocusRange << std::endl;
						break;
						case GLFW_KEY_LEFT:
						dofFocusRange += dofFocusStep;
						std::cout << "Range: " << dofFocusRange << std::endl;
						break;
						case GLFW_KEY_T:
						dofTesting = !dofTesting;
						std::cout << "Testing: " << (dofTesting ? "True" : "False") << std::endl;
					}
				}
			});

		OGLMaterialFactory materialFactory;
		materialFactory.loadShadersFromDir("../../../projects/depthOfField/shaders/");
		materialFactory.loadTexturesFromDir("../../../data/textures/");

		OGLGeometryFactory geometryFactory;


		std::array<SimpleScene, 1> scenes {
			createCottageScene(materialFactory, geometryFactory),
		};

		Renderer renderer(materialFactory);
		window.onResize([&camera, &window, &renderer](int width, int height) {
				camera.setAspectRatio(window.aspectRatio());
				renderer.initialize(width, height);
			});


		renderer.initialize(window.size()[0], window.size()[1]);
		window.runLoop([&] {
			renderer.shadowMapPass(scenes[config.currentSceneIdx], light);

			renderer.clear();
			renderer.geometryPass(scenes[config.currentSceneIdx], camera, RenderOptions{"solid"});
			renderer.compositingPass(light);
			renderer.depthOfFieldPass(dofFocusDistance, dofFocusRange, dofTesting);
		});
	} catch (ShaderCompilationError &exc) {
		std::cerr
			<< "Shader compilation error!\n"
			<< "Shader type: " << exc.shaderTypeName()
			<<"\nError: " << exc.what() << "\n";
		return -3;
	} catch (OpenGLError &exc) {
		std::cerr << "OpenGL error: " << exc.what() << "\n";
		return -2;
	} catch (std::exception &exc) {
		std::cerr << "Error: " << exc.what() << "\n";
		return -1;
	}

	glfwTerminate();
	return 0;
}
