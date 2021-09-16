// SPDX-FileCopyrightText: 2017-2021 Alexander Ulmer <alexulmer1999@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <syscalls.h>
#include <context.h>
#include <lib/errno.h>
#include <debug.h>

#include "../../kernel/syscall_list.h"

void do_syscall(IrqContext* ctx)
{
  /* check for an invalid system call number */
  if (ctx->eax >= sizeof(syscalls) / sizeof(void*))
  {
    ctx->eax = -ENOSYS;
    return;
  }

  /* resolve the function address of the system call */
  void* syscall_addr = syscalls[ctx->eax];
  if (syscall_addr == nullptr)
  {
    ctx->eax = -ENOSYS;
    return;
  }

  auto syscall = (size_t(*)(
        size_t arg1, size_t arg2, size_t arg3,
        size_t arg4, size_t arg5, size_t arg6
  ))syscall_addr;

  /* actually perform the call with the arguments from
   * the interrupt context structure */
  ctx->eax = syscall(
    ctx->ebx, ctx->ecx, ctx->edx,
    ctx->edi, ctx->esi,  ctx->ebp
  );
}
