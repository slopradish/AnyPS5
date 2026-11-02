#include "prx/libkernel/include/Api.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libkernel/KernelErrors.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

static_assert(offsetof(SceKernelProsperoSystemSwVersion, size) == 0x00);
static_assert(offsetof(SceKernelProsperoSystemSwVersion, versionString) == 0x08);
static_assert(offsetof(SceKernelProsperoSystemSwVersion, version) == 0x24);
static_assert(sizeof(SceKernelProsperoSystemSwVersion) == 0x28);

namespace {

constexpr std::uint32_t SystemSoftwareVersion = 0x01000000;
constexpr char SystemSoftwareVersionString[] = "01.000.000";
constexpr std::uint32_t SdkPatchMask = 0xfff;

}

extern "C" {

int APS5_VABI sceKernelGetProsperoSystemSwVersion_nid_postfix(SceKernelProsperoSystemSwVersion* version) {
    if (version == nullptr) return 0;
    std::memset(version->versionString, 0, sizeof(version->versionString));
    std::memcpy(version->versionString, SystemSoftwareVersionString, sizeof(SystemSoftwareVersionString));
    version->version = SystemSoftwareVersion;
    return 0;
}

int APS5_VABI sceKernelGetAllowedSdkVersionOnSystem_nid_postfix(std::uint32_t* version) {
    if (version == nullptr) return SCE_KERNEL_ERROR_EINVAL;
    *version = SystemSoftwareVersion | SdkPatchMask;
    return 0;
}

int APS5_VABI sceKernelGetProsperoCompiledSdkVersion(char* compiledSdkVersion) {
    if (compiledSdkVersion == nullptr) {
        return 0;
    }
    const auto* parameters = static_cast<const std::byte*>(ApplicationProcessParameters_nid_no_patch());
    std::uint32_t version;
    std::memcpy(&version, parameters + 0x10, sizeof(version));
    std::snprintf(compiledSdkVersion, 24, "%02u.%03u.%03u.%03u", (version >> 24) & 0xffu,
                  (version >> 16) & 0xffu, (version >> 8) & 0xffu, version & 0xffu);
    return 0;
}

}
