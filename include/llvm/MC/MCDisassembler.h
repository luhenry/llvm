//===-- llvm/MC/MCDisassembler.h - Disassembler interface -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_MC_MCDISASSEMBLER_H
#define LLVM_MC_MCDISASSEMBLER_H

#include "llvm-c/Disassembler.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/MC/MCRelocationInfo.h"
#include "llvm/MC/MCSymbolizer.h"
#include "llvm/Support/DataTypes.h"

namespace llvm {

class MCInst;
class MCSubtargetInfo;
class raw_ostream;
class MCContext;

/// Superclass for all disassemblers. Consumes a memory region and provides an
/// array of assembly instructions.
class MCDisassembler {
public:
  /// Ternary decode status. Most backends will just use Fail and
  /// Success, however some have a concept of an instruction with
  /// understandable semantics but which is architecturally
  /// incorrect. An example of this is ARM UNPREDICTABLE instructions
  /// which are disassemblable but cause undefined behaviour.
  ///
  /// Because it makes sense to disassemble these instructions, there
  /// is a "soft fail" failure mode that indicates the MCInst& is
  /// valid but architecturally incorrect.
  ///
  /// The enum numbers are deliberately chosen such that reduction
  /// from Success->SoftFail ->Fail can be done with a simple
  /// bitwise-AND:
  ///
  ///   LEFT & TOP =  | Success       Unpredictable   Fail
  ///   --------------+-----------------------------------
  ///   Success       | Success       Unpredictable   Fail
  ///   Unpredictable | Unpredictable Unpredictable   Fail
  ///   Fail          | Fail          Fail            Fail
  ///
  /// An easy way of encoding this is as 0b11, 0b01, 0b00 for
  /// Success, SoftFail, Fail respectively.
  enum DecodeStatus {
    Fail = 0,
    SoftFail = 1,
    Success = 3
  };

  MCDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx)
    : Ctx(Ctx), STI(STI), Symbolizer(), CommentStream(nullptr) {}

  virtual ~MCDisassembler();

  /// Returns the disassembly of a single instruction.
  ///
  /// @param Instr    - An MCInst to populate with the contents of the
  ///                   instruction.
  /// @param Size     - A value to populate with the size of the instruction, or
  ///                   the number of bytes consumed while attempting to decode
  ///                   an invalid instruction.
  /// @param Region   - The memory object to use as a source for machine code.
  /// @param Address  - The address, in the memory space of region, of the first
  ///                   byte of the instruction.
  /// @param VStream  - The stream to print warnings and diagnostic messages on.
  /// @param CStream  - The stream to print comments and annotations on.
  /// @return         - MCDisassembler::Success if the instruction is valid,
  ///                   MCDisassembler::SoftFail if the instruction was
  ///                                            disassemblable but invalid,
  ///                   MCDisassembler::Fail if the instruction was invalid.
  virtual DecodeStatus getInstruction(MCInst &Instr, uint64_t &Size,
                                      ArrayRef<uint8_t> Bytes, uint64_t Address,
                                      raw_ostream &VStream,
                                      raw_ostream &CStream) const = 0;

private:
  MCContext &Ctx;

protected:
  // Subtarget information, for instruction decoding predicates if required.
  const MCSubtargetInfo &STI;
  std::unique_ptr<MCSymbolizer> Symbolizer;

public:
  // Helpers around MCSymbolizer
  bool tryAddingSymbolicOperand(MCInst &Inst,
                                int64_t Value,
                                uint64_t Address, bool IsBranch,
                                uint64_t Offset, uint64_t InstSize) const;

  void tryAddingPcLoadReferenceComment(int64_t Value, uint64_t Address) const;

  /// Set \p Symzer as the current symbolizer.
  /// This takes ownership of \p Symzer, and deletes the previously set one.
  void setSymbolizer(std::unique_ptr<MCSymbolizer> Symzer);

  MCContext& getContext() const { return Ctx; }

  const MCSubtargetInfo& getSubtargetInfo() const { return STI; }

  // Marked mutable because we cache it inside the disassembler, rather than
  // having to pass it around as an argument through all the autogenerated code.
  mutable raw_ostream *CommentStream;
};

} // namespace llvm

#endif
