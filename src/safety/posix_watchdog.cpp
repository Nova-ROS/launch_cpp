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
 * @file posix_watchdog.cpp
 * @brief POSIX implementation of Watchdog
 * 
 * ASIL: B (Simple adapter - code reviewed)
 * Monitors node health via heartbeat mechanism with timeout detection.
 */

#include "osal.hpp"
#include <map>
#include <mutex>
#include <chrono>
#include <thread>
#include <functional>
#include <atomic>
#include <algorithm>

namespace cpp_launch {


// ============================================================================
// Watchdog Node State
// ============================================================================

struct WatchdogNode {
    uint32_t node_id{0};
    uint32_t timeout_ms{0};
    HeartbeatCallback heartbeat_callback;
    std::chrono::steady_clock::time_point last_heartbeat;
    std::chrono::steady_clock::time_point registration_time;
    uint32_t sequence{0};
    bool responsive{true};
    ProcessState last_state{ProcessState::kUnknown};
};

// ============================================================================
// PosixWatchdog Implementation
// ============================================================================

class PosixWatchdog::Impl {
public:
    Impl() : running_(false) {}
    ~Impl() { StopInternal(); }

    OsalResult<void> RegisterNodeInternal(
        uint32_t node_id,
        uint32_t timeout_ms,
        HeartbeatCallback callback);

    OsalResult<void> UnregisterNodeInternal(uint32_t node_id);
    OsalResult<void> SubmitHeartbeatInternal(const HeartbeatMessage& message);
    OsalResult<bool> IsResponsiveInternal(uint32_t node_id);
    void SetTimeoutCallbackInternal(std::function<void(uint32_t)> callback);
    OsalResult<void> StartInternal();
    OsalResult<void> StopInternal();

private:
    void WatchdogThread();
    bool ValidateHeartbeat(const HeartbeatMessage& message);
    void CheckTimeouts();
    
    std::map<uint32_t, WatchdogNode> nodes_;
    std::mutex nodes_mutex_;
    std::function<void(uint32_t)> timeout_callback_;
    std::thread watchdog_thread_;
    std::atomic<bool> running_;
    std::chrono::steady_clock::time_point start_time_;
};

// ============================================================================
// Public Interface
// ============================================================================

PosixWatchdog::PosixWatchdog() 
    : impl_(std::make_unique<Impl>()) {}

PosixWatchdog::~PosixWatchdog() = default;

OsalResult<void> PosixWatchdog::RegisterNode(
    uint32_t node_id,
    uint32_t timeout_ms,
    HeartbeatCallback callback) {
    return impl_->RegisterNodeInternal(node_id, timeout_ms, callback);
}

OsalResult<void> PosixWatchdog::UnregisterNode(uint32_t node_id) {
    return impl_->UnregisterNodeInternal(node_id);
}

OsalResult<void> PosixWatchdog::SubmitHeartbeat(const HeartbeatMessage& message) {
    return impl_->SubmitHeartbeatInternal(message);
}

OsalResult<bool> PosixWatchdog::IsResponsive(uint32_t node_id) {
    return impl_->IsResponsiveInternal(node_id);
}

void PosixWatchdog::SetTimeoutCallback(std::function<void(uint32_t)> callback) {
    impl_->SetTimeoutCallbackInternal(callback);
}

OsalResult<void> PosixWatchdog::Start() {
    return impl_->StartInternal();
}

OsalResult<void> PosixWatchdog::Stop() {
    return impl_->StopInternal();
}

// ============================================================================
// Implementation Details
// ============================================================================

OsalResult<void> PosixWatchdog::Impl::RegisterNodeInternal(
    uint32_t node_id,
    uint32_t timeout_ms,
    HeartbeatCallback callback) {
    
    if (timeout_ms == 0) {
        return OsalResult<void>(
            OsalStatus::kInvalidArgument,
            "Timeout must be greater than 0");
    }
    
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    // Check if node already registered
    if (nodes_.find(node_id) != nodes_.end()) {
        return OsalResult<void>(
            OsalStatus::kError,
            "Node already registered");
    }
    
    WatchdogNode node;
    node.node_id = node_id;
    node.timeout_ms = timeout_ms;
    node.heartbeat_callback = callback;
    node.last_heartbeat = std::chrono::steady_clock::now();
    node.registration_time = node.last_heartbeat;
    node.sequence = 0;
    node.responsive = true;
    node.last_state = ProcessState::kStarting;
    
    nodes_[node_id] = node;
    
    return OsalResult<void>();
}

OsalResult<void> PosixWatchdog::Impl::UnregisterNodeInternal(uint32_t node_id) {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return OsalResult<void>(
            OsalStatus::kNotFound,
            "Node not found");
    }
    
    nodes_.erase(it);
    return OsalResult<void>();
}

OsalResult<void> PosixWatchdog::Impl::SubmitHeartbeatInternal(
    const HeartbeatMessage& message) {
    
    // Validate checksum
    if (!ValidateHeartbeat(message)) {
        return OsalResult<void>(
            OsalStatus::kInvalidArgument,
            "Invalid heartbeat checksum");
    }
    
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    auto it = nodes_.find(message.node_id);
    if (it == nodes_.end()) {
        return OsalResult<void>(
            OsalStatus::kNotFound,
            "Node not registered");
    }
    
    auto& node = it->second;
    
    // Check sequence number (detect missing heartbeats)
    if (message.sequence != node.sequence + 1 && message.sequence != 0) {
        // Sequence gap detected - could log warning
    }
    
    // Update node state
    node.sequence = message.sequence;
    node.last_heartbeat = std::chrono::steady_clock::now();
    node.last_state = message.state;
    node.responsive = true;
    
    // Invoke callback if registered
    if (node.heartbeat_callback) {
        node.heartbeat_callback(message);
    }
    
    return OsalResult<void>();
}

OsalResult<bool> PosixWatchdog::Impl::IsResponsiveInternal(uint32_t node_id) {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) {
        return OsalResult<bool>(
            OsalStatus::kNotFound,
            "Node not found");
    }
    
    return OsalResult<bool>(it->second.responsive);
}

void PosixWatchdog::Impl::SetTimeoutCallbackInternal(
    std::function<void(uint32_t)> callback) {
    timeout_callback_ = callback;
}

OsalResult<void> PosixWatchdog::Impl::StartInternal() {
    if (running_.exchange(true)) {
        return OsalResult<void>(
            OsalStatus::kError,
            "Watchdog already running");
    }
    
    start_time_ = std::chrono::steady_clock::now();
    watchdog_thread_ = std::thread(&Impl::WatchdogThread, this);
    
    return OsalResult<void>();
}

OsalResult<void> PosixWatchdog::Impl::StopInternal() {
    if (!running_.exchange(false)) {
        return OsalResult<void>();  // Already stopped
    }
    
    if (watchdog_thread_.joinable()) {
        watchdog_thread_.join();
    }
    
    return OsalResult<void>();
}

void PosixWatchdog::Impl::WatchdogThread() {
    while (running_) {
        CheckTimeouts();
        
        // Check every 100ms
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void PosixWatchdog::Impl::CheckTimeouts() {
    auto now = std::chrono::steady_clock::now();
    
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    
    for (auto& [node_id, node] : nodes_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - node.last_heartbeat).count();
        
        if (elapsed > static_cast<int64_t>(node.timeout_ms)) {
            // Timeout detected
            if (node.responsive) {
                node.responsive = false;
                
                // Invoke timeout callback
                if (timeout_callback_) {
                    timeout_callback_(node_id);
                }
            }
        }
    }
}

bool PosixWatchdog::Impl::ValidateHeartbeat(const HeartbeatMessage& message) {
    // Simple checksum validation
    uint32_t expected = message.CalculateChecksum();
    return (message.checksum == expected);
}

// namespace cpp_launch
