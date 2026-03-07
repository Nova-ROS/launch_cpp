// Copyright 2025 Example Author
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
 * @file mock_osal.cpp
 * @brief Mock implementations for unit testing
 * 
 * ASIL: Not used in production - for testing only
 */

#include "osal.hpp"
#include <mutex>
#include <map>
#include <atomic>

namespace ara {
namespace exec {

// ============================================================================
// MockProcessExecutor Implementation
// ============================================================================

MockProcessExecutor::MockProcessExecutor() = default;
MockProcessExecutor::~MockProcessExecutor() = default;

void MockProcessExecutor::SetExecuteCallback(ExecuteCallback callback) {
    execute_cb_ = callback;
}

void MockProcessExecutor::SetWaitCallback(WaitCallback callback) {
    wait_cb_ = callback;
}

void MockProcessExecutor::SetIsRunningCallback(IsRunningCallback callback) {
    is_running_cb_ = callback;
}

void MockProcessExecutor::SetTerminateCallback(TerminateCallback callback) {
    terminate_cb_ = callback;
}

void MockProcessExecutor::SetKillCallback(KillCallback callback) {
    kill_cb_ = callback;
}

void MockProcessExecutor::SetSendSignalCallback(SendSignalCallback callback) {
    send_signal_cb_ = callback;
}

void MockProcessExecutor::SetGetStateCallback(GetStateCallback callback) {
    get_state_cb_ = callback;
}

OsalResult<ProcessId> MockProcessExecutor::Execute(
    const CommandLine& command,
    const ProcessOptions& options) {
    
    if (execute_cb_) {
        return execute_cb_(command, options);
    }
    
    // Default: return a mock PID
    static std::atomic<ProcessId> next_pid{1000};
    return OsalResult<ProcessId>(next_pid.fetch_add(1));
}

OsalResult<ProcessResult> MockProcessExecutor::Wait(
    ProcessId pid,
    std::chrono::milliseconds timeout) {
    
    if (wait_cb_) {
        return wait_cb_(pid, timeout);
    }
    
    // Default: return success
    ProcessResult result;
    result.pid = pid;
    result.exit_code = 0;
    result.final_state = ProcessState::kStopped;
    return OsalResult<ProcessResult>(result);
}

OsalResult<bool> MockProcessExecutor::IsRunning(ProcessId pid) {
    if (is_running_cb_) {
        return is_running_cb_(pid);
    }
    return OsalResult<bool>(false);
}

OsalResult<void> MockProcessExecutor::Terminate(
    ProcessId pid,
    std::chrono::milliseconds timeout) {
    
    if (terminate_cb_) {
        return terminate_cb_(pid, timeout);
    }
    return OsalResult<void>();
}

OsalResult<void> MockProcessExecutor::Kill(ProcessId pid) {
    if (kill_cb_) {
        return kill_cb_(pid);
    }
    return OsalResult<void>();
}

OsalResult<void> MockProcessExecutor::SendSignal(ProcessId pid, int32_t signal) {
    if (send_signal_cb_) {
        return send_signal_cb_(pid, signal);
    }
    return OsalResult<void>();
}

OsalResult<ProcessState> MockProcessExecutor::GetState(ProcessId pid) {
    if (get_state_cb_) {
        return get_state_cb_(pid);
    }
    return OsalResult<ProcessState>(ProcessState::kUnknown);
}

} // namespace exec
} // namespace ara
