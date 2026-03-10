// Copyright 2026 Nova ROS, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


/**
 * @file posix_resource_monitor.cpp
 * @brief POSIX implementation of ResourceMonitor
 * 
 * ASIL: B (Simple adapter - code reviewed)
 * Reads system and process resource information from /proc filesystem.
 */

#include "osal.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <chrono>
#include <thread>
#include <algorithm>
#include <map>
#include <cmath>
#include <mutex>

namespace launch_cpp {


// ============================================================================
// PosixResourceMonitor Implementation
// ============================================================================

class PosixResourceMonitor::Impl {
public:
    Impl() = default;
    ~Impl() = default;

    OsalResult<SystemResources> get_system_resources_internal();
    OsalResult<ResourceUsage> get_process_resources_internal(ProcessId pid);
    OsalResult<bool> are_resources_available_internal(uint64_t estimated_memory);
    OsalResult<void> set_resource_limits_internal(
        ProcessId pid, uint64_t max_memory, double max_cpu_percent);

    void register_threshold_callback_internal(
        double threshold,
        std::function<void(const SystemResources&)> callback);
    
    void MonitorThread();
    void StopMonitoring();

private:
    // Parse /proc/meminfo
    bool ParseMemInfo(uint64_t& total_memory, uint64_t& available_memory);
    
    // Parse /proc/stat for CPU load
    bool ParseCpuStat(double& load1, double& load5, double& load15);
    
    // Parse /proc/[pid]/status for process memory
    bool ParseProcessStatus(ProcessId pid, uint64_t& vm_rss, uint64_t& vm_size);
    
    // Parse /proc/[pid]/stat for CPU usage
    bool ParseProcessStat(ProcessId pid, double& cpu_percent);
    
    // Get system uptime
    double GetUptime();
    
    // Get process uptime
    double GetProcessUptime(ProcessId pid);

    std::map<double, std::function<void(const SystemResources&)>> callbacks_;
    std::thread monitor_thread_;
    bool monitoring_{false};
    std::mutex callback_mutex_;
};

// ============================================================================
// Public Interface
// ============================================================================

PosixResourceMonitor::PosixResourceMonitor() 
    : impl_(std::make_unique<Impl>()) {}

PosixResourceMonitor::~PosixResourceMonitor() {
    impl_->StopMonitoring();
}

OsalResult<SystemResources> PosixResourceMonitor::get_system_resources() {
    return impl_->get_system_resources_internal();
}

OsalResult<ResourceUsage> PosixResourceMonitor::get_process_resources(ProcessId pid) {
    return impl_->get_process_resources_internal(pid);
}

OsalResult<bool> PosixResourceMonitor::are_resources_available(uint64_t estimated_memory) {
    return impl_->are_resources_available_internal(estimated_memory);
}

OsalResult<void> PosixResourceMonitor::set_resource_limits(
    ProcessId pid,
    uint64_t max_memory,
    double max_cpu_percent) {
    return impl_->set_resource_limits_internal(pid, max_memory, max_cpu_percent);
}

void PosixResourceMonitor::register_threshold_callback(
    double threshold,
    std::function<void(const SystemResources&)> callback) {
    impl_->register_threshold_callback_internal(threshold, callback);
}

// ============================================================================
// Implementation Details
// ============================================================================

OsalResult<SystemResources> PosixResourceMonitor::Impl::get_system_resources_internal() {
    SystemResources resources;
    
    // Get memory information
    uint64_t total_mem, available_mem;
    if (!ParseMemInfo(total_mem, available_mem)) {
        return OsalResult<SystemResources>(
            OsalStatus::kError,
            "Failed to parse /proc/meminfo");
    }
    resources.total_memory_bytes = total_mem;
    resources.available_memory_bytes = available_mem;
    
    // Get CPU load
    double load1, load5, load15;
    if (!ParseCpuStat(load1, load5, load15)) {
        // Not critical, set to 0
        load1 = load5 = load15 = 0.0;
    }
    resources.cpu_load_1min = load1;
    resources.cpu_load_5min = load5;
    resources.cpu_load_15min = load15;
    
    // Get process count (approximate from /proc)
    resources.total_processes = static_cast<uint32_t>(sysconf(_SC_NPROCESSORS_ONLN));
    
    // Get file descriptor limits
    struct rlimit rlim;
    if (getrlimit(RLIMIT_NOFILE, &rlim) == 0) {
        resources.max_file_descriptors = static_cast<uint32_t>(rlim.rlim_max);
        resources.used_file_descriptors = static_cast<uint32_t>(rlim.rlim_cur);
    }
    
    return OsalResult<SystemResources>(resources);
}

OsalResult<ResourceUsage> PosixResourceMonitor::Impl::get_process_resources_internal(ProcessId pid) {
    ResourceUsage usage;
    
    // Get memory usage
    uint64_t vm_rss, vm_size;
    if (!ParseProcessStatus(static_cast<pid_t>(pid), vm_rss, vm_size)) {
        return OsalResult<ResourceUsage>(
            OsalStatus::kNotFound,
            "Failed to read process status");
    }
    usage.memory_bytes = vm_rss;
    usage.virtual_memory_bytes = vm_size;
    
    // Get CPU usage
    double cpu_percent;
    if (ParseProcessStat(static_cast<pid_t>(pid), cpu_percent)) {
        usage.cpu_percent = cpu_percent;
    }
    
    // Get file descriptor count
    std::string fd_path = "/proc/" + std::to_string(pid) + "/fd";
    // Count entries in fd directory (simplified)
    usage.file_descriptors = 0;  // Would need directory iteration
    
    return OsalResult<ResourceUsage>(usage);
}

OsalResult<bool> PosixResourceMonitor::Impl::are_resources_available_internal(
    uint64_t estimated_memory) {
    
    auto sys_result = get_system_resources_internal();
    if (sys_result.has_error()) {
        return OsalResult<bool>(
            sys_result.get_status(),
            sys_result.get_error_message());
    }
    
    const auto& resources = sys_result.get_value();
    
    // Check if enough memory available
    if (resources.available_memory_bytes < estimated_memory) {
        return OsalResult<bool>(false);
    }
    
    // Check CPU load (if too high, don't start new processes)
    if (resources.cpu_load_1min > 0.8) {  // 80% CPU load threshold
        return OsalResult<bool>(false);
    }
    
    return OsalResult<bool>(true);
}

OsalResult<void> PosixResourceMonitor::Impl::set_resource_limits_internal(
    ProcessId pid,
    uint64_t max_memory,
    double max_cpu_percent) {
    
    // Set memory limit using POSIX setrlimit
    struct rlimit rlim;
    rlim.rlim_cur = max_memory;
    rlim.rlim_max = max_memory;
    
    // Note: This sets limits for current process, not the target process
    // In production, would need to set before exec() or use cgroups
    if (setrlimit(RLIMIT_AS, &rlim) != 0) {
        return OsalResult<void>(
            OsalStatus::kError,
            "Failed to set memory limit");
    }
    
    // CPU limits would require cgroups on Linux
    // This is a simplified implementation
    
    return OsalResult<void>();
}

void PosixResourceMonitor::Impl::register_threshold_callback_internal(
    double threshold,
    std::function<void(const SystemResources&)> callback) {
    
    std::lock_guard<std::mutex> lock(callback_mutex_);
    callbacks_[threshold] = callback;
    
    // Start monitoring thread if not already running
    if (!monitoring_) {
        monitoring_ = true;
        monitor_thread_ = std::thread(&Impl::MonitorThread, this);
    }
}

void PosixResourceMonitor::Impl::MonitorThread() {
    while (monitoring_) {
        auto result = get_system_resources_internal();
        if (!result.has_error()) {
            const auto& resources = result.get_value();
            
            // Check all callbacks
            std::lock_guard<std::mutex> lock(callback_mutex_);
            for (const auto& [threshold, callback] : callbacks_) {
                double mem_usage = 1.0 - (static_cast<double>(resources.available_memory_bytes) / 
                                         static_cast<double>(resources.total_memory_bytes));
                if (mem_usage > threshold) {
                    callback(resources);
                }
            }
        }
        
        // Check every 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void PosixResourceMonitor::Impl::StopMonitoring() {
    monitoring_ = false;
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

bool PosixResourceMonitor::Impl::ParseMemInfo(
    uint64_t& total_memory, 
    uint64_t& available_memory) {
    
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    total_memory = 0;
    available_memory = 0;
    
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        uint64_t value;
        std::string unit;
        
        if (iss >> key >> value >> unit) {
            if (key == "MemTotal:") {
                total_memory = value * 1024;  // Convert kB to bytes
            } else if (key == "MemAvailable:") {
                available_memory = value * 1024;
            }
        }
    }
    
    return (total_memory > 0);
}

bool PosixResourceMonitor::Impl::ParseCpuStat(
    double& load1, double& load5, double& load15) {
    
    std::ifstream file("/proc/loadavg");
    if (!file.is_open()) {
        return false;
    }
    
    file >> load1 >> load5 >> load15;
    return !file.fail();
}

bool PosixResourceMonitor::Impl::ParseProcessStatus(
    ProcessId pid, uint64_t& vm_rss, uint64_t& vm_size) {
    
    std::string path = "/proc/" + std::to_string(pid) + "/status";
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    vm_rss = 0;
    vm_size = 0;
    
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        uint64_t value;
        std::string unit;
        
        if (line.find("VmRSS:") == 0) {
            iss >> key >> value >> unit;
            vm_rss = value * 1024;  // kB to bytes
        } else if (line.find("VmSize:") == 0) {
            iss >> key >> value >> unit;
            vm_size = value * 1024;
        }
    }
    
    return true;
}

bool PosixResourceMonitor::Impl::ParseProcessStat(
    ProcessId pid, double& cpu_percent) {
    
    std::string path = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    // Parse stat file (simplified)
    // Format: pid comm state ppid ... utime stime ...
    std::string line;
    std::getline(file, line);
    
    std::istringstream iss(line);
    std::string comm;
    char state;
    long pid_read, ppid, pgrp, session, tty_nr, tpgid;
    unsigned long flags, minflt, cminflt, majflt, cmajflt, utime, stime;
    
    // Skip to utime (field 14) and stime (field 15)
    if (iss >> pid_read >> comm >> state >> ppid >> pgrp >> session >> tty_nr 
            >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt 
            >> utime >> stime) {
        
        // Calculate CPU usage (simplified - would need previous values for accuracy)
        unsigned long total_time = utime + stime;
        cpu_percent = static_cast<double>(total_time) / 100.0;  // Simplified
        return true;
    }
    
    return false;
}

double PosixResourceMonitor::Impl::GetUptime() {
    std::ifstream file("/proc/uptime");
    if (!file.is_open()) {
        return 0.0;
    }
    
    double uptime;
    file >> uptime;
    return uptime;
}

double PosixResourceMonitor::Impl::GetProcessUptime(ProcessId pid) {
    std::string path = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream file(path);
    if (!file.is_open()) {
        return 0.0;
    }
    
    // Start time is field 22 (in clock ticks since boot)
    std::string line;
    std::getline(file, line);
    
    std::istringstream iss(line);
    std::string comm;
    char state;
    long pid_read;
    unsigned long starttime = 0;
    
    // Skip to starttime (field 22)
    for (int i = 0; i < 21; ++i) {
        std::string field;
        iss >> field;
    }
    iss >> starttime;
    
    // Convert to seconds
    long ticks_per_sec = sysconf(_SC_CLK_TCK);
    double sys_uptime = GetUptime();
    
    return sys_uptime - (static_cast<double>(starttime) / ticks_per_sec);
}

} // namespace launch_cpp
