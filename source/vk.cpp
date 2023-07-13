

#include <string>
#include <array>
#include <vector>

using namespace std;
using namespace vk;

namespace {
    bool vulkanSupported = true;

    const char* APPLICATION_NAME = "Endless Sky";
    const uint32_t APPLICATION_VERSION = VK_MAKE_API_VERSION(0, 0, 10, 2);
    const char* ENGINE_NAME = "No Engine";
    const uint32_t ENGINE_VERSION = VK_MAKE_API_VERSION(0, 0, 0, 1);

    const array<const char *, 0> enabledExtentions = {};
    const array<const char *, 0> enabledLayers = {};


    Instance instance;

    void CreateInstance(vector<const char *> extentions) {
        ApplicationInfo appInfo(
            APPLICATION_NAME,
            APPLICATION_VERSION,
            ENGINE_NAME,
            ENGINE_VERSION,
            VK_API_VERSION_1_0
        );

        for(const char* ext : enabledExtentions)
        {
            extentions.push_back(ext);
        }

        InstanceCreateInfo createInfo(
            InstanceCreateFlags(),
            &appInfo,
            enabledLayers,
            extentions
        );

        instance = createInstance(createInfo);
    };
}

void Vulkan::Initialize(SDL_Window *window)
{
    unsigned extentionCount;
	SDL_Vulkan_GetInstanceExtensions(window, &extentionCount, nullptr);
	vector<const char *> extentions(extentionCount);
	SDL_Vulkan_GetInstanceExtensions(window, &extentionCount, extentions.data());
	if(extentions.size() != extentionCount)
		throw runtime_error("Failed to write Vulkan instance extentions successfully.");

    CreateInstance(extentions);
}



// constexpr const ApplicationInfo Vulkan::ApplicationInfo()
// {
//     vk::ApplicationInfo appInfo(
//         APPLICATION_NAME,
        
//     );

    

//     return appInfo;
// }
