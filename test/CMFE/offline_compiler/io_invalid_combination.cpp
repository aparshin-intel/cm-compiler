/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

SPDX-License-Identifier: MIT
============================= end_copyright_notice ===========================*/

// RUN: %cmc -emit-spirv   -o %t.spv -mcpu=SKL -- %s
// RUN: %cmc -emit-llvm    -o %t.bc -mcpu=SKL -- %s
// RUN: %cmc -emit-llvm -S -o %t.ll -mcpu=SKL -- %s

// RUN: %cmc -emit-spirv -o output 2>&1 -- %t.spv \
// RUN:         | FileCheck %s
//
// RUN: %cmc -emit-llvm -o output 2>&1 -- %t.bc \
// RUN:         | FileCheck %s

// RUN: %cmc -emit-llvm -S -o output 2>&1 -- %t.bc \
// RUN:         | FileCheck %s
//

// CHECK: not supported


#include <cm/cm.h>

extern "C" _GENX_MAIN_
void test_kernel() {
}

