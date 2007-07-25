// Copyright (c) 2006, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// stackwalker_x86.h: x86-specific stackwalker.
//
// Provides stack frames given x86 register context and a memory region
// corresponding to an x86 stack.
//
// Author: Mark Mentovai


#ifndef PROCESSOR_STACKWALKER_X86_H__
#define PROCESSOR_STACKWALKER_X86_H__


#include "google_airbag/common/airbag_types.h"
#include "google_airbag/common/minidump_format.h"
#include "google_airbag/processor/stackwalker.h"

namespace google_airbag {

class MinidumpContext;
class MinidumpModuleList;


class StackwalkerX86 : public Stackwalker {
 public:
  // context is a MinidumpContext object that gives access to x86-specific
  // register state corresponding to the innermost called frame to be
  // included in the stack.  The other arguments are passed directly through
  // to the base Stackwalker constructor.
  StackwalkerX86(const MDRawContextX86 *context,
                 MemoryRegion *memory,
                 MinidumpModuleList *modules,
                 SymbolSupplier *supplier);

 private:
  // Implementation of Stackwalker, using x86 context (%ebp, %esp, %eip) and
  // stack conventions (saved %ebp at [%ebp], saved %eip at 4[%ebp], or
  // alternate conventions as guided by stack_frame_info_).
  virtual StackFrame* GetContextFrame();
  virtual StackFrame* GetCallerFrame(
      const CallStack *stack,
      const vector< linked_ptr<StackFrameInfo> > &stack_frame_info);

  // Stores the CPU context corresponding to the innermost stack frame to
  // be returned by GetContextFrame.
  const MDRawContextX86 *context_;
};


}  // namespace google_airbag


#endif  // PROCESSOR_STACKWALKER_X86_H__
