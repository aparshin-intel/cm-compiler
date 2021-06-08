/*========================== begin_copyright_notice ============================

Copyright (C) 2016-2021 Intel Corporation

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

// For some reason this test fails if the RUN lines are at the
// end of the file - why?
// RUN: %cmc -emit-llvm -- %s 2>&1 | FileCheck -allow-empty --implicit-check-not error %s

#include <cm/cm.h>

_GENX_ void test1()
{
  ushort offsetx = 0, offsety = 0, sizex = 0;

  vector<int, 16> cond;
  vector<int, 16> v;
  // ...
  SIMD_IF_BEGIN (cond) {
    // ...
  } SIMD_ELSE {
    SIMD_IF_BEGIN (cond < 0) {
      vector<int, 16> local;
      // ...
      local = v.select<16, 1>(offsetx + offsety * sizex);
      // ...
    } SIMD_ELSE {
      // ...
    } SIMD_IF_END;
  } SIMD_IF_END;
  // ...
  SIMD_DO_WHILE_BEGIN {
    vector<int, 16> local;
    // ...
    SIMD_IF_BEGIN (local < 0) {
      SIMD_BREAK;
    } SIMD_IF_END;
    // ...
  } SIMD_DO_WHILE_END (cond < 32);
  // ...
}
