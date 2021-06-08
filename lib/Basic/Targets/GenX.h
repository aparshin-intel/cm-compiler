/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

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

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

// This file declares Intel GenX TargetInfo objects.

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_GENX_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_GENX_H

#include "OSTargets.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/Compiler.h"

namespace clang {
namespace targets {

// GenX
// Not all features of a target are required for GenX.
// We are primarily interested in being able to specify a particular gpu
// variant.
//
class LLVM_LIBRARY_VISIBILITY GenXTargetInfo : public TargetInfo {
  static const Builtin::Info BuiltinInfo[];
  std::string CPU;
  bool OCLRuntime = false;
  bool NativeI64Support = false;
  bool NativeDoubleSupport = false;

public:
  GenXTargetInfo(const llvm::Triple &Triple, unsigned PointerWidth);

  /// \brief Flags for architecture specific defines.
  typedef enum {
    ArchDefineNone = 0,
    ArchDefineName = 1 << 0 // <name> is substituted for arch name.
  } ArchDefineTypes;

  bool setCPU(const std::string &Name) override;

  const std::string &getCPU() const override { return CPU; }

  ArrayRef<Builtin::Info> getTargetBuiltins() const override {
    return ArrayRef<Builtin::Info>();
  }

  BuiltinVaListKind getBuiltinVaListKind() const override {
    return TargetInfo::VoidPtrBuiltinVaList;
  }

  void getTargetDefines(const LangOptions &Opts,
                        MacroBuilder &Builder) const override;

  bool handleTargetFeatures(std::vector<std::string> &Features,
                            DiagnosticsEngine &Diags) override;

  bool hasFeature(StringRef Feature) const override;

  virtual const char *getClobbers() const { return ""; }

  ArrayRef<const char *> getGCCRegNames() const override {
    return ArrayRef<const char *>();
  }

  ArrayRef<GCCRegAlias> getGCCRegAliases() const override {
    return ArrayRef<GCCRegAlias>();
  }

  // Convert 'cr' and 'rw' into '^cr' and '^rw'.
  // '^' is a hint for two-letter constraint
  std::string convertConstraint(const char *&Constraint) const override {
    if ((Constraint[0] == 'r' && Constraint[1] == 'w') ||
        (Constraint[0] == 'c' && Constraint[1] == 'r'))
      return std::string("^") + std::string(Constraint++, 2);
    return std::string(1, *Constraint);
  }

  bool validateAsmConstraint(const char *&Name,
                             TargetInfo::ConstraintInfo &Info) const override {
    switch (*Name) {
    default:
      return false;
    case 'c':
      Name++;
      switch (*Name) {
      default:
        return false;
      case 'r':
        Info.setAllowsRegister();
        return true;
      }
    case 'a':
      Info.setAllowsRegister();
      return true;
    case 'w':
      Info.setAllowsRegister();
      return true;
    }
  }
};

} // namespace targets
} // namespace clang
#endif // LLVM_CLANG_LIB_BASIC_TARGETS_GENX_H
