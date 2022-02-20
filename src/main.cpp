#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <exception>
#include <stdexcept>
#include <vector>
#include <cstdlib>
#include <cstring>

constexpr auto g_window_width = 800;
constexpr auto g_window_height = 600;

const auto g_validation_layers =
		std::vector<const char*>{"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
const auto g_enable_validation_layers = false;
#else
const auto g_enable_validation_layers = true;
#endif

auto check_validation_layer_support() -> bool {
	auto layer_count = uint32_t{};
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
	auto available_layers = std::vector<VkLayerProperties>(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
	for (const auto* layer_name : g_validation_layers) {
		auto layer_found = false;
		for (const auto& layer_properties : available_layers) {
			if (strcmp(
							layer_name,
							static_cast<const char*>(layer_properties.layerName)) == 0) {
				layer_found = true;
				break;
			}
		}
		if (!layer_found) {
			return false;
		}
	}
	return true;
}

class VulkanApplication {
 public:
	void run() {
		init_window();
		init_vulkan();
		loop();
		cleanup();
	}

 private:
	GLFWwindow* _window;
	VkInstance _instance;

	void init_window() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		_window = glfwCreateWindow(
				g_window_width,
				g_window_height,
				"vulkan-demo",
				nullptr,
				nullptr);
	}

	void init_vulkan() {
		create_instance();
	}

	void create_instance() {
		if (g_enable_validation_layers && !check_validation_layer_support()) {
			throw std::runtime_error("validation layers requested but not available");
		}
		auto create_info = VkInstanceCreateInfo{};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		auto extension_count = uint32_t{};
		const auto** extensions =
				glfwGetRequiredInstanceExtensions(&extension_count);
		create_info.enabledExtensionCount = extension_count;
		create_info.ppEnabledExtensionNames = extensions;
		if (g_enable_validation_layers) {
			create_info.enabledLayerCount =
					static_cast<uint32_t>(g_validation_layers.size());
			create_info.ppEnabledLayerNames = g_validation_layers.data();
		}
		if (vkCreateInstance(&create_info, nullptr, &_instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create vulkan instance");
		}
	}

	void loop() {
		while (glfwWindowShouldClose(_window) == 0) {
			glfwPollEvents();
		}
	}

	void cleanup() {
		vkDestroyInstance(_instance, nullptr);
		glfwDestroyWindow(_window);
		glfwTerminate();
	}
};

auto main() -> int {
	auto app = VulkanApplication{};
	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	return EXIT_SUCCESS;
}
