#ifndef VULKAN_QNX_H_
#define VULKAN_QNX_H_ 1

#ifdef __cplusplus
extern "C" {
#endif


#define VK_QNX_screen_surface 1

#define VK_QNX_SCREEN_SURFACE_SPEC_VERSION 1
#define VK_QNX_SCREEN_SURFACE_EXTENSION_NAME "VK_QNX_screen_surface"
#define VK_STRUCTURE_TYPE_SCREEN_SURFACE_CREATE_INFO_QNX 1000378000

typedef VkFlags VkScreenSurfaceCreateFlagsQNX;

typedef struct VkScreenSurfaceCreateInfoQNX {
    VkStructureType                   sType;
    const void*                       pNext;
    VkScreenSurfaceCreateFlagsQNX     flags;
    struct _screen_context*           context;
    struct _screen_window*            window;
} VkScreenSurfaceCreateInfoQNX;

typedef VkResult (VKAPI_PTR *PFN_vkCreateScreenSurfaceQNX)(VkInstance instance, const VkScreenSurfaceCreateInfoQNX* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
typedef VkBool32 (VKAPI_PTR *PFN_vkGetPhysicalDeviceScreenPresentationSupportQNX)(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, struct _screen_window* window);

#ifdef __cplusplus
}
#endif

#endif
