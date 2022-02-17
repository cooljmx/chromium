// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/policy/linux/bpf_utility_policy_linux.h"

#include <errno.h>

#include "build/build_config.h"
#include "sandbox/linux/bpf_dsl/bpf_dsl.h"
#include "sandbox/linux/seccomp-bpf-helpers/syscall_parameters_restrictions.h"
#include "sandbox/linux/seccomp-bpf-helpers/syscall_sets.h"
#include "sandbox/linux/system_headers/linux_syscalls.h"
#include "sandbox/policy/linux/sandbox_linux.h"

using sandbox::bpf_dsl::Allow;
using sandbox::bpf_dsl::Error;
using sandbox::bpf_dsl::ResultExpr;

namespace sandbox {
namespace policy {

UtilityProcessPolicy::UtilityProcessPolicy() {}
UtilityProcessPolicy::~UtilityProcessPolicy() {}

ResultExpr UtilityProcessPolicy::EvaluateSyscall(int sysno) const {
  switch (sysno) {
    case __NR_seccomp:
    case __NR_sendfile:
    case __NR_socket:
    case __NR_connect:
    case __NR_accept:
    case __NR_sendto:
    case __NR_recvfrom:
    case __NR_sendmsg:
    case __NR_recvmsg:
    case __NR_shutdown:
    case __NR_bind:
    case __NR_listen:
    case __NR_getsockname:
    case __NR_getpeername:
    case __NR_socketpair:
    case __NR_setsockopt:
    case __NR_getsockopt:
    case __NR_wait4:
    case __NR_kill:
      return Error(EINVAL);
    default:
      return Allow();
  }

}

}  // namespace policy
}  // namespace sandbox
