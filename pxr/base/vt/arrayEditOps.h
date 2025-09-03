//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_VT_ARRAY_EDIT_OPS_H
#define PXR_BASE_VT_ARRAY_EDIT_OPS_H

/// \file vt/arrayEditOps.h

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"

#include "pxr/base/tf/hash.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// A helper class used in the implementation of VtArrayEdit.
class Vt_ArrayEditOps
{
public:
    static constexpr int64_t EndIndex = std::numeric_limits<int64_t>::min();
    
    // The supported operations.
    //
    // This enum's underlying type is int64_t because MSVC won't pack it
    // correctly when used as a bitfield in a struct.  For example, on MSVC if
    // we make the underlying type uint8_t, then struct Foo { int64_t x:56; Op:8
    // }; has size 16.  Making the underlying type int64_t fixes this.
    enum Op : int64_t {
        OpWriteLiteral,  // write <literal> to [index]
        OpWriteRef,      // write [index1] to [index2]
        OpInsertLiteral, // insert <literal> at [index]
        OpInsertRef,     // insert [index1] at [index2]
        OpEraseRef,      // erase [index]
        OpMinSize,       // minsize <size>
        OpMinSizeFill,   // minsize <size> <literal>
        OpSetSize,       // resize <size>
        OpSetSizeFill,   // resize <size> <literal>
        OpMaxSize,       // maxsize <size>
    };
    static constexpr uint8_t NumOps = OpMaxSize + 1;
    
    static constexpr bool IsValidOp(Op op) {
        return op >= static_cast<Op>(0) && op < NumOps;
    }

    static constexpr int GetArity(Op op) {
        if (op == OpWriteLiteral || op == OpWriteRef ||
            op == OpInsertLiteral || op == OpInsertRef ||
            op == OpMinSizeFill || op == OpSetSizeFill) {
            return 2;
        }
        return 1;
    }

    // Repetitions of a given op.
    struct OpAndCount {
        int64_t count:56;
        Op op:8;
    };
    static_assert(sizeof(OpAndCount) == sizeof(int64_t));

    // Invoke fn(Op, arg1, arg2) for each valid instruction.  Normalize index
    // args according to initialSize (if negative or EndIndex).  Invalid
    // instructions with out-of-bounds indexes are skipped.
    template <class Fn>
    void ForEachValid(size_t numLiterals, size_t initialSize, Fn &&fn) const {
        return _ForEachImpl(numLiterals, initialSize, _ins,
                            std::forward<Fn>(fn));
    }

    // Invoke fn(Op, arg1, arg2) for each instruction as-is, with no index
    // normalization or range checking.
    template <class Fn>
    void ForEach(Fn &&fn) const {
        return _ForEachImpl(-1, -1, _ins, std::forward<Fn>(fn));
    }
        
    // Invoke fn(Op, arg1, arg2) for each instruction as-is, with no index
    // normalization or range checking, and passing mutable references for arg1
    // and arg2 to fn().  This lets fn() modify indexes if desired.
    template <class Fn>
    void ModifyEach(Fn &&fn) {
        return _ForEachImpl(-1, -1, _ins, std::forward<Fn>(fn));
    }

    // Return true if there are no ops, else false.
    bool IsEmpty() const {
        return _ins.empty();
    }

private:
    template <class ELEM>
    friend class VtArrayEdit;

    template <class ELEM>
    friend class VtArrayEditBuilder;

    friend class Vt_ArrayEditOpsBuilder;

    template <class HashState>
    friend void TfHashAppend(HashState &h, Vt_ArrayEditOps const &self) {
        h.Append(self._ins);
    }

    friend bool
    operator==(Vt_ArrayEditOps const &l, Vt_ArrayEditOps const &r) {
        return l._ins == r._ins;
    }
    friend bool
    operator!=(Vt_ArrayEditOps const &l, Vt_ArrayEditOps const &r) {
        return !(l == r);
    }
    
    static OpAndCount _ToOpAndCount(int64_t i64) {
        OpAndCount oc;
        memcpy(&oc, &i64, sizeof(oc));
        return oc;
    }

    static int64_t _ToInt64(OpAndCount oc) {
        int64_t i64;
        memcpy(&i64, &oc, sizeof(i64));
        return i64;
    }

    static constexpr size_t _DisableBoundsCheck = size_t(-1);

    VT_API
    void _IssueInvalidInsError(
        OpAndCount oc, size_t offset, size_t required, size_t actual) const;

    VT_API
    void _IssueInvalidOpError(OpAndCount oc, size_t offset) const;

    VT_API static void _LiteralOutOfBounds(int64_t idx, size_t size);
    VT_API static void _ReferenceOutOfBounds(int64_t idx, size_t size);
    VT_API static void _InsertOutOfBounds(int64_t idx, size_t size);
    VT_API static void _NegativeSizeArg(Op op, int64_t idx);
   
    static bool _CheckLiteralIndex(int64_t idx, size_t size) {
        if (size == _DisableBoundsCheck ||
            (idx >= 0 && static_cast<size_t>(idx) < size)) {
            return true;
        }
        _LiteralOutOfBounds(idx, size);
        return false;
    }
    
    static bool _NormalizeAndCheckRefIndex(int64_t &idx, size_t size) {
        if (size == _DisableBoundsCheck) {
            return true;
        }
        if (idx < 0) {
            idx += size;
        }
        if (idx >= 0 && static_cast<size_t>(idx) < size) {
            return true;
        }
        _ReferenceOutOfBounds(idx, size);
        return false;
    }
    
    static bool _NormalizeAndCheckInsertIndex(int64_t &idx, size_t size) {
        if (size == _DisableBoundsCheck) {
            return true;
        }
        if (idx == EndIndex) {
            idx = size;
        }
        if (idx < 0) {
            idx += size;
        }
        if (idx >= 0 && static_cast<size_t>(idx) <= size) {
            return true;
        }
        _InsertOutOfBounds(idx, size);
        return false;
    }

    static bool _CheckSizeArg(Op op, int64_t arg) {
        if (arg < 0) {
            _NegativeSizeArg(op, arg);
            return false;
        }
        return true;
    }

    static void _UpdateWorkingSize(size_t &workingSize, size_t newSize) {
        if (workingSize == _DisableBoundsCheck) {
            return;
        }
        workingSize = newSize;
    }
    
    template <class Ins, class Fn>
    void _ForEachImpl(size_t numLiterals, size_t initialSize,
                      Ins &&ins, Fn &&fn) const {

        // Walk and call fn with each.
        const auto begin = std::begin(std::forward<Ins>(ins));
        auto iter = std::begin(std::forward<Ins>(ins));
        const auto end = std::end(std::forward<Ins>(ins));

        while (iter != end) {
            OpAndCount oc = _ToOpAndCount(*iter);

            if (!IsValidOp(oc.op)) {
                _IssueInvalidOpError(oc, std::distance(begin, iter));
                return;
            }

            const int arity = GetArity(oc.op);

            // Check sufficient args.
            if (std::distance(++iter, end) < oc.count * arity) {
                _IssueInvalidInsError(
                    oc, std::distance(begin, iter),
                    oc.count * arity, std::distance(iter, end));
            }

            // Do each set of args.
            for (; oc.count--; iter += arity) {
                int64_t a1 = iter[0];
                int64_t a2 = arity > 1 ? iter[1] : -1;

                // Normalize and check indexes if requested.
                switch (oc.op) {
                default:
                    _IssueInvalidOpError(oc, std::distance(begin, iter));
                    return;
                case OpWriteLiteral:
                    if (!_CheckLiteralIndex(a1, numLiterals) ||
                        !_NormalizeAndCheckRefIndex(a2, initialSize)) {
                        continue;
                    }
                    break;
                case OpWriteRef:
                    if (!_NormalizeAndCheckRefIndex(a1, initialSize) ||
                        !_NormalizeAndCheckRefIndex(a2, initialSize)) {
                        continue;
                    }
                    break;
                case OpInsertLiteral:
                    if (!_CheckLiteralIndex(a1, numLiterals) ||
                        !_NormalizeAndCheckInsertIndex(a2, initialSize)) {
                        continue;
                    }
                    _UpdateWorkingSize(initialSize, initialSize + 1);
                    break;
                case OpInsertRef:
                    if (!_NormalizeAndCheckRefIndex(a1, initialSize) ||
                        !_NormalizeAndCheckInsertIndex(a2, initialSize)) {
                        continue;
                    }
                    _UpdateWorkingSize(initialSize, initialSize + 1);
                    break;
                case OpEraseRef:
                    if (!_NormalizeAndCheckRefIndex(a1, initialSize)) {
                        continue;
                    }
                    _UpdateWorkingSize(initialSize, initialSize - 1);
                    break;
                    
                case OpMinSizeFill:
                    if (!_CheckLiteralIndex(a2, numLiterals)) {
                        continue;
                    } // intentional fall-thru
                case OpMinSize:
                    if (!_CheckSizeArg(oc.op, a1)) {
                        continue;
                    }
                    _UpdateWorkingSize(
                        initialSize,
                        std::max(initialSize, static_cast<size_t>(a1)));
                    break;
                                        
                case OpSetSizeFill:
                    if (!_CheckLiteralIndex(a2, numLiterals)) {
                        continue;
                    } // intentional fall-thru
                case OpSetSize:
                    if (!_CheckSizeArg(oc.op, a1)) {
                        continue;
                    }
                    _UpdateWorkingSize(initialSize, a1);
                    break;
                    
                case OpMaxSize:
                    if (!_CheckSizeArg(oc.op, a1)) {
                        continue;
                    }
                    _UpdateWorkingSize(
                        initialSize,
                        std::min(initialSize, static_cast<size_t>(a1)));
                    break;
                    
                };

                // Invoke caller.
                std::forward<Fn>(fn)(oc.op, a1, a2);
            }
        }
    }

    // Instructions and arguments are stored together in order.  For example the
    // following operation sequence:
    //
    // resize 1024
    // write <literal 0> to [2]
    // write <literal 1> to [4]
    // write [5] to [6]
    // erase [9]
    // erase [9]
    //
    // Would be encoded as the following 64-bit quantities, each denoted by [].
    //
    // [1 OpSetSize] [1024] [2 OpWriteLiteral] [0] [2] [1] [4] [1 OpWriteRef]
    // [5] [6] [2 OpErase] [9] [9].
    //
    // The meaning of the 64-bit quantities that follow an op are determined by
    // the op itself.  For example, in the case of [2 OpWriteLiteral], there are
    // four quantities that follow, two for each instruction: a literal index
    // and a destination index.  The GetArity() member function returns the
    // arity for a given op.  Currently either 1 or 2.
    std::vector<int64_t> _ins;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_ARRAY_EDIT_OPS_H
