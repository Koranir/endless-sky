/* vulkan.cpp
Copyright (c) 2023 by Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "vk.h"

#include "SDL2/SDL_vulkan.h"

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
