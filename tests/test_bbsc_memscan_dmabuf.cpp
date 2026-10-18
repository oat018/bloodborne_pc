// SPDX-License-Identifier: GPL-2.0-or-later
// Explicit hardware regression: no game or window; requires cached dma-buf export support.
#include "core/bbsc_memscan.h"
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>
#include <vulkan/vulkan.h>

int main() {
    constexpr std::size_t Piece = 1u << 20, Size = Piece * 2, Offset = Piece - 2;
    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo instance_info{};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app;
    VkInstance instance{};
    assert(vkCreateInstance(&instance_info, nullptr, &instance) == VK_SUCCESS);
    uint32_t count = 0;
    assert(vkEnumeratePhysicalDevices(instance, &count, nullptr) == VK_SUCCESS && count > 0);
    std::vector<VkPhysicalDevice> devices(count);
    assert(vkEnumeratePhysicalDevices(instance, &count, devices.data()) == VK_SUCCESS);
    VkPhysicalDevice physical = devices.front();
    for (const auto candidate : devices) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(candidate, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            physical = candidate;
            break;
        }
    }
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical, &properties);
    std::printf("GPU: %s\n", properties.deviceName);
    float priority = 1;
    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = 0;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;
    const char* extensions[] = {VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
                                VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME};
    VkDeviceCreateInfo device_info{};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = 2;
    device_info.ppEnabledExtensionNames = extensions;
    VkDevice device{};
    assert(vkCreateDevice(physical, &device_info, nullptr, &device) == VK_SUCCESS);
    VkExternalMemoryBufferCreateInfo external{};
    external.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
    external.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.pNext = &external;
    buffer_info.size = Size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer buffer{};
    assert(vkCreateBuffer(device, &buffer_info, nullptr, &buffer) == VK_SUCCESS);
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, buffer, &requirements);
    VkPhysicalDeviceMemoryProperties memory_properties{};
    vkGetPhysicalDeviceMemoryProperties(physical, &memory_properties);
    uint32_t type = ~0u;
    constexpr auto Wanted = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
        const auto flags = memory_properties.memoryTypes[i].propertyFlags;
        if ((requirements.memoryTypeBits & (1u << i)) && (flags & Wanted) == Wanted &&
            !(flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            type = i;
            break;
        }
    }
    assert(type != ~0u);
    VkExportMemoryAllocateInfo export_info{};
    export_info.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
    export_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.pNext = &export_info;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = type;
    VkDeviceMemory memory{};
    assert(vkAllocateMemory(device, &allocate_info, nullptr, &memory) == VK_SUCCESS);
    const auto get_fd = reinterpret_cast<PFN_vkGetMemoryFdKHR>(
        vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR"));
    assert(get_fd);
    VkMemoryGetFdInfoKHR fd_info{};
    fd_info.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    fd_info.memory = memory;
    fd_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
    int fd = -1;
    assert(get_fd(device, &fd_info, &fd) == VK_SUCCESS);
    auto* data = static_cast<unsigned char*>(
        mmap(nullptr, Size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    assert(data != MAP_FAILED);
    constexpr std::array<unsigned char, 6> Needle{0x79, 0x31, 0x94, 0x62, 0x88, 0x2a};
    std::memcpy(data + Offset, Needle.data(), Needle.size());
    std::memcpy(data + Piece + 4096, Needle.data(), Needle.size());
    std::array<unsigned char, 6> got{};
    iovec local{got.data(), got.size()}, remote{data + Offset, got.size()};
    errno = 0;
    auto n = process_vm_readv(getpid(), &local, 1, &remote, 1, 0);
    std::printf("process_vm_readv: %zd errno %d\n", n, errno);
    const int self = open("/proc/self/mem", O_RDONLY | O_CLOEXEC);
    assert(self >= 0);
    errno = 0;
    n = pread(self, got.data(), got.size(), reinterpret_cast<off_t>(data + Offset));
    std::printf("/proc/self/mem pread: %zd errno %d\n", n, errno);
    assert(n == ssize_t(Needle.size()) && got == Needle);
    assert(close(self) == 0);
    const auto lo = reinterpret_cast<std::uint64_t>(data);
    const auto hits = Core::Bbsc::FindInMemory(Needle, lo, lo + Size, 16);
    assert((hits == std::vector<std::uint64_t>{lo + Offset, lo + Piece + 4096}));
    assert(munmap(data, Size) == 0);
    assert(close(fd) == 0);
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    std::puts("PASS: BBSC scan finds param bytes in real exported dma-buf memory");
}
