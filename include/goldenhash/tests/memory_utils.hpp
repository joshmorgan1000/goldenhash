#pragma once
#include <cstdint>

#ifdef __linux__
#include <sys/sysinfo.h>
#elif __APPLE__
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#endif

namespace goldenhash::tests {

/**
 * @brief Get available system memory
 * @return Available memory in bytes
 */
inline uint64_t get_available_memory() {
#ifdef __linux__
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        return si.freeram + si.bufferram;
    }
#elif __APPLE__
    vm_size_t page_size;
    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t host_size = sizeof(vm_stat) / sizeof(natural_t);
    
    if (host_page_size(mach_host_self(), &page_size) == KERN_SUCCESS &&
        host_statistics64(mach_host_self(), HOST_VM_INFO,
                         (host_info64_t)&vm_stat, &host_size) == KERN_SUCCESS) {
        return (vm_stat.free_count + vm_stat.inactive_count) * page_size;
    }
#endif
    return 4ULL * 1024 * 1024 * 1024; // Default to 4GB if can't determine
}

} // namespace goldenhash
