#pragma once

#include <optional>
#include <tuple>

#include <vulkan/vulkan.h>
#include <spdlog/spdlog.h>

#include "globals.h"
#include "result.h"

struct QueueFamilyIndices 
{
    int32_t graphicsFamily;

    QueueFamilyIndices()
    {
        graphicsFamily = -1;
    }

    bool isComplete() 
    {
        return (graphicsFamily > -1);
    }
};

struct PhysicalDevice 
{
    VkPhysicalDevice vkPyhsicalDevice;
    QueueFamilyIndices queueFamilyIndices;
};

struct RenderContext
{
    PhysicalDevice physicalDevice;
};

Result<QueueFamilyIndices> findQueueFamilies(VkPhysicalDevice device) 
{
    QueueFamilyIndices indices;
    
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
        {
            indices.graphicsFamily = i;
        }

        if (indices.isComplete()) 
        {
            return Result<QueueFamilyIndices>(KIFF_SUCCESS, std::make_unique<QueueFamilyIndices>(indices));
            break;
        }

        i++;
    }

    return Result<QueueFamilyIndices>(KIFF_COULD_NOT_FIND_QUEUE_FAMILY_INDICES);
}

Result<PhysicalDevice> selectPhysicalDevice(VkInstance instance)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) 
    {
        return Result<PhysicalDevice>(KIFF_ERROR);
    }

    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());
    
#ifndef NDEBUG
    spdlog::info(DEBUG_MSG_PHYSICAL_DEVICE_PRESENT);
    for (uint32_t i = 0; i < deviceCount; ++i) 
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
        
        spdlog::info(DEBUG_INDENT_1, deviceProperties.deviceName);
    }
#endif

    for (uint32_t i = 0; i < deviceCount; ++i) 
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(physicalDevices[i], &deviceFeatures);

        auto indices = findQueueFamilies(physicalDevices[i]);
        
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader && indices.getCode() == KIFF_SUCCESS)
        {
#ifndef NDEBUG
            spdlog::info(DEBUG_MSG_SELECTED_DEVICE, deviceProperties.deviceName);
#endif

            auto physical_device = PhysicalDevice{
                .vkPyhsicalDevice = physicalDevices[i],
                .queueFamilyIndices = indices.getValue()
            };
            return Result<PhysicalDevice>(KIFF_SUCCESS, std::make_unique<PhysicalDevice>(physical_device));
        }

        return Result<PhysicalDevice>(KIFF_SUITIBLE_PHYSICAL_DEVICE_NOT_FOUND);
    }

    return Result<PhysicalDevice>(KIFF_UNKNOWN);
}

Result<VkDevice> createLogicalDevice(PhysicalDevice physicalDevice)
{
    VkDeviceQueueCreateInfo queue_create_info{};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = physicalDevice.queueFamilyIndices.graphicsFamily;
    queue_create_info.queueCount = 1;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo device_create_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_create_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = nullptr,
        .pEnabledFeatures = &deviceFeatures,
    };

    VkDevice device;
    auto result = vkCreateDevice(physicalDevice.vkPyhsicalDevice, &device_create_info, nullptr, &device);

    if (result == VK_SUCCESS)
    {
        return Result<VkDevice>(KIFF_SUCCESS, std::make_unique<VkDevice>(device));
    }

    return Result<VkDevice>(KIFF_CREATE_DEVICE_FAILED);
}
