//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "spirvTransforms.h"

#include "pxr/base/tf/span.h"
#include "pxr/base/trace/trace.h"

#include <spirv-tools/libspirv.hpp>
#include <spirv/unified1/GLSL.std.450.h>
#include <spirv/unified1/spirv.h>

#include <array>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HGIWEBGPU_DEBUG_SPIRV_TRANSFORM, "Dump SPIR-V after transformation");
}

using Word = uint32_t;
using Operand = Word;
using Id = Word;
using Literal = TfSpan<const Word>;

class _SpvByteCode;

/// Extend this class and override the functions to receive parsed
/// instructions from a SPIR-V module. The functions are called in
/// order following the logical layout rules. Keep the functions
/// sorted this way too. See:
/// https://registry.khronos.org/SPIR-V/specs/unified1/SPIRV.html#_logical_layout_of_a_module
/// This only covers the opcode we use now, add more when necessary.
class _SpvVisitor
{
public:
    explicit _SpvVisitor(_SpvByteCode& byteCode)
        : _byteCode{byteCode}
    {
    }

    virtual ~_SpvVisitor() = default;

    virtual void OpExtInstImport(Id result, std::string_view name) {}

    virtual void OpEntryPoint(SpvExecutionModel executionModel, Id entryPoint,
        std::string_view name, TfSpan<const Operand> variables)
    {
    }

    virtual void OpExecutionMode(Id entryPoint, SpvExecutionMode executionMode,
        const std::vector<Literal>& extra)
    {
    }

    virtual void OpDecorate(
        Id target, SpvDecoration decoration, const std::vector<Literal>& extra)
    {
    }

    virtual void OpMemberDecorate(Id structType, Word member,
        SpvDecoration decoration, const std::vector<Literal>& extra)
    {
    }

    virtual void OpTypeInt(Id result, Word width, Word isSigned) {}

    virtual void OpTypeFloat(
        Id result, Word width, std::optional<Operand> encoding)
    {
    }

    virtual void OpTypeBool(Id result) {}

    virtual void OpTypeVector(Id result, Id componentType, Word count) {}

    virtual void OpTypeArray(Id result, Id elementType, Id length) {}

    virtual void OpTypePointer(
        Id result, SpvStorageClass storageClass, Id pointeeType)
    {
    }

    virtual void OpConstant(Id resultType, Id result, Literal value) {}

    virtual void OpVariable(Id resultType, Id result,
        SpvStorageClass storageClass, std::optional<Operand> initializer)
    {
    }

    /// This is called once before the first OpFunction()
    virtual void BeginFunctions() {}

    virtual void OpFunction(Id resultType, Id result,
        SpvFunctionControlMask functionControl, Id functionType)
    {
    }

    virtual void OpFunctionEnd() {}

    virtual void OpLabel(Id result) {}

protected:
    /// An instruction is copied to the result
    /// bytecode before the callback is called.
    /// Use the mutation function to edit the bytecode.
    _SpvByteCode& _byteCode;
};

/// Represents a SPIR-V bytecode to transform.
/// The transformations are done on a copy.
class _SpvByteCode
{
public:
    explicit _SpvByteCode(const std::vector<Word>& spirv)
        : _oldSpirv{spirv}
    {
        _newSpirv.reserve(_oldSpirv.size());
        _spvTools.SetMessageConsumer(&_PrintMessage);
    }

    /// Convenience overload.
    template<typename VisitorType>
    bool Apply()
    {
        VisitorType visitor{*this};
        return Apply(visitor);
    }

    /// Parse the bytecode and apply a transformation.
    /// Copies the instructions one at a time, and calls
    /// the corresponding _SpvVisitor virtual member function
    /// after each supported instruction is copied.
    bool Apply(_SpvVisitor& visitor)
    {
        _firstFunctionSeen = false;
        _spvTools.Parse(
            _oldSpirv,
            [this](const spv_endianness_t endianness,
                const spv_parsed_header_t& header) {
                return _ParseHeader(endianness, header);
            },
            [this, &visitor](const spv_parsed_instruction_t& inst) {
                return _ParseInstruction(visitor, inst);
            });

        _newSpirv[offsetof(spv_parsed_header_t, bound) / sizeof(Word)] =
            _nextId;

        if (ARCH_UNLIKELY(
                TfDebug::IsEnabled(HGIWEBGPU_DEBUG_SPIRV_TRANSFORM))) {
            std::string disassembly;
            _spvTools.Disassemble(_newSpirv, &disassembly);
            TF_DEBUG(HGIWEBGPU_DEBUG_SPIRV_TRANSFORM)
                .Msg("--- BEGIN SPIR-V DISASSEMBLY ---\n%s"
                     "\n---- END SPIR-V DISASSEMBLY ----\n",
                    disassembly.c_str());
        }

        if (ARCH_UNLIKELY(!_spvTools.Validate(_newSpirv))) {
            TF_CODING_ERROR("SPIR-V transformation failed");
            return false;
        }

        return true;
    }

    /// Inject an instruction immediately after the current instruction,
    /// and any previously injected instruction.
    void Inject(SpvOp op, std::initializer_list<Word> words)
    {
        const auto opCode = (static_cast<Word>(words.size() + 1) << 16) |
            (static_cast<Word>(op) & 0xffff);
        _newSpirv.insert(_newSpirv.end(), opCode);
        _newSpirv.insert(_newSpirv.end(), words);
    }

    /// Modify a word in the bytecode.
    void Modify(size_t offset, Word word) { _newSpirv.at(offset) = word; }

    /// Word offset of the current instruction in the bytecode.
    /// This is the position of the first instruction word in the vector.
    size_t CurrentOffset() const { return _offset; }

    /// Allocate a new <id>.
    Id NextId() { return _nextId++; }

    /// Insert a word by appending it to the current instruction.
    void InsertWord(Word word)
    {
        _newSpirv.push_back(word);
        Word& header = _newSpirv.at(_offset);
        header = (((header >> 16) + 1) << 16) | (header & 0xffff);
    }

    /// Move out the bytecode transformation output.
    std::vector<Word> TakeResult() { return std::move(_newSpirv); }

    /// Get a word sized int from a literal.
    static Word LiteralAsInt(Literal literal)
    {
        if (ARCH_UNLIKELY(literal.size() != 1)) {
            TF_CODING_ERROR("Literal must be one word");
        }

        return literal.front();
    }

    /// Get an enum from a literal.
    template<typename T>
    static T LiteralAsEnum(Literal literal)
    {
        static_assert(std::is_enum_v<T>, "T must be an enum");
        return static_cast<T>(LiteralAsInt(literal));
    }

    /// Get a UTF-8 string from a literal.
    static std::string_view LiteralAsString(Literal literal)
    {
        const auto chars = reinterpret_cast<const char*>(literal.data());
        const auto length = strnlen(chars, literal.size() * sizeof(Word));
        return {chars, length};
    }

private:
    static void _PrintMessage(spv_message_level_t level, const char* source,
        const spv_position_t& position, const char* message)
    {
        switch (level) {
        case SPV_MSG_FATAL:
            TF_FATAL_ERROR("%s:%lu -- %s", source, position.index, message);
            break;
        case SPV_MSG_INTERNAL_ERROR:
        case SPV_MSG_ERROR:
            TF_RUNTIME_ERROR("%s:%lu -- %s", source, position.index, message);
            break;
        case SPV_MSG_WARNING:
            TF_WARN("%s:%lu -- %s", source, position.index, message);
            break;
        case SPV_MSG_INFO:
        case SPV_MSG_DEBUG:
            break;
        }
    }

    spv_result_t _ParseHeader(
        const spv_endianness_t, const spv_parsed_header_t& header)
    {
        _nextId = header.bound;
        _newSpirv.resize(sizeof(header) / sizeof(Word));
        std::memcpy(_newSpirv.data(), &header, sizeof(header));
        return SPV_SUCCESS;
    }

    spv_result_t _ParseInstruction(
        _SpvVisitor& visitor, const spv_parsed_instruction_t& inst)
    {
        if (!_firstFunctionSeen && inst.opcode == SpvOpFunction) {
            _firstFunctionSeen = true;
            visitor.BeginFunctions();
        }

        _offset = _newSpirv.size();
        _newSpirv.insert(
            _newSpirv.end(), inst.words, inst.words + inst.num_words);

        size_t opIndex = 0;
        const auto nextOperand = [&inst, &opIndex] {
            return inst.words[inst.operands[opIndex++].offset];
        };
        const auto optionalOperand = [&inst, &opIndex, &nextOperand] {
            return opIndex < inst.num_operands ? std::optional{nextOperand()} :
                                                 std::nullopt;
        };
        const auto remainingOperands = [&inst, &opIndex] {
            // One word per operand
            const auto firstWordOffset =
                static_cast<size_t>(inst.operands[opIndex].offset);
            opIndex = inst.num_operands;
            return TfSpan{
                inst.words + firstWordOffset, inst.num_words - firstWordOffset};
        };
        const auto nextLiteral = [&inst, &opIndex] {
            const auto& operand = inst.operands[opIndex++];
            return Literal{inst.words + operand.offset, operand.num_words};
        };
        const auto remainingLiterals = [&inst, &opIndex, &nextLiteral] {
            std::vector<Literal> literals;
            for (size_t i = opIndex; i < inst.num_operands; i++) {
                literals.push_back(nextLiteral());
            }
            return literals;
        };
        const auto skipInstruction = [&inst, &opIndex] {
            opIndex = inst.num_operands;
        };

        // The order of evaluation of arguments in a C++ function call is
        // unspecified. We need the operands to be parsed in order from the
        // word stream. Order of evaluation is guaranteed for list-init, so
        // we can wrap the arguments in a std::tuple using braces (important!),
        // which will parse the operands in the order they appear in the list,
        // then call the visitor function using std::apply, which unwraps the
        // tuple and forwards the arguments.
        // https://en.cppreference.com/w/cpp/language/eval_order
        const auto visitWithOperands = [&visitor](auto&& func, auto&& args) {
            std::apply(std::forward<decltype(func)>(func),
                std::tuple_cat(std::forward_as_tuple(visitor),
                    std::forward<decltype(args)>(args)));
        };

        switch (inst.opcode) {
        case SpvOpExtInstImport:
            visitWithOperands(&_SpvVisitor::OpExtInstImport,
                std::tuple{nextOperand(), LiteralAsString(nextLiteral())});
            break;
        case SpvOpEntryPoint:
            visitWithOperands(&_SpvVisitor::OpEntryPoint,
                std::tuple{static_cast<SpvExecutionModel>(nextOperand()),
                    nextOperand(), LiteralAsString(nextLiteral()),
                    remainingOperands()});
            break;
        case SpvOpExecutionMode:
            visitWithOperands(&_SpvVisitor::OpExecutionMode,
                std::tuple{nextOperand(),
                    static_cast<SpvExecutionMode>(nextOperand()),
                    remainingLiterals()});
            break;
        case SpvOpDecorate:
            visitWithOperands(&_SpvVisitor::OpDecorate,
                std::tuple{nextOperand(),
                    static_cast<SpvDecoration>(nextOperand()),
                    remainingLiterals()});
            break;
        case SpvOpMemberDecorate:
            visitWithOperands(&_SpvVisitor::OpMemberDecorate,
                std::tuple{nextOperand(), LiteralAsInt(nextLiteral()),
                    static_cast<SpvDecoration>(nextOperand()),
                    remainingLiterals()});
            break;
        case SpvOpTypeInt:
            visitWithOperands(&_SpvVisitor::OpTypeInt,
                std::tuple{nextOperand(), LiteralAsInt(nextLiteral()),
                    LiteralAsInt(nextLiteral())});
            break;
        case SpvOpTypeFloat:
            visitWithOperands(&_SpvVisitor::OpTypeFloat,
                std::tuple{nextOperand(), LiteralAsInt(nextLiteral()),
                    optionalOperand()});
            break;
        case SpvOpTypeBool:
            visitor.OpTypeBool(nextOperand());
            break;
        case SpvOpTypeVector:
            visitWithOperands(&_SpvVisitor::OpTypeVector,
                std::tuple{
                    nextOperand(), nextOperand(), LiteralAsInt(nextLiteral())});
            break;
        case SpvOpTypeArray:
            visitWithOperands(&_SpvVisitor::OpTypeArray,
                std::tuple{nextOperand(), nextOperand(), nextOperand()});
            break;
        case SpvOpTypePointer:
            visitWithOperands(&_SpvVisitor::OpTypePointer,
                std::tuple{nextOperand(),
                    static_cast<SpvStorageClass>(nextOperand()),
                    nextOperand()});
            break;
        case SpvOpConstant:
            visitWithOperands(&_SpvVisitor::OpConstant,
                std::tuple{nextOperand(), nextOperand(), nextLiteral()});
            break;
        case SpvOpVariable:
            visitWithOperands(&_SpvVisitor::OpVariable,
                std::tuple{nextOperand(), nextOperand(),
                    static_cast<SpvStorageClass>(nextOperand()),
                    optionalOperand()});
            break;
        case SpvOpFunction:
            visitWithOperands(&_SpvVisitor::OpFunction,
                std::tuple{nextOperand(), nextOperand(),
                    static_cast<SpvFunctionControlMask>(nextOperand()),
                    nextOperand()});
            break;
        case SpvOpFunctionEnd:
            visitor.OpFunctionEnd();
            break;
        case SpvOpLabel:
            visitor.OpLabel(nextOperand());
            break;
        default:
            skipInstruction();
            break;
        }

        if (ARCH_UNLIKELY(opIndex != inst.num_operands)) {
            TF_CODING_ERROR("Operands were not evaluated correctly, "
                            "parsed %lu operands instead of %u",
                opIndex, inst.num_operands);
        }

        return SPV_SUCCESS;
    }

    const std::vector<Word>& _oldSpirv;
    std::vector<Word> _newSpirv;

    spvtools::SpirvTools _spvTools{SPV_ENV_VULKAN_1_0};

    size_t _offset = 0;
    Id _nextId = 0;
    bool _firstFunctionSeen = false;
};

/// Common base class to handle common data types and their constants. Add more
/// as necessary.
///
/// Call _Ensure*Type() and _EnsureConst*() in BeginFunctions() to inject a type
/// and constant value respectively, if it was not already found in the code.
/// Call _GetConst*() in OpFunctionEnd() (or any point after BeginFunctions())
/// to retrieve a constant that was already injected (errors if missing by
/// default).
class _SpvTypeCollector : public _SpvVisitor
{
public:
    using _SpvVisitor::_SpvVisitor;

    void OpTypeBool(Id result) override
    {
        if (!_boolTypeId) {
            _boolTypeId = result;
        }
    }

    void OpTypeInt(Id result, Word width, Word isSigned) override
    {
        if (width == 32) {
            if (isSigned == 1 && !_intTypeId) {
                _intTypeId = result;
            } else if (isSigned == 0 && !_uintTypeId) {
                _uintTypeId = result;
            }
        }
    }

    void OpTypeFloat(Id result, Word width, std::optional<Operand>) override
    {
        if (width == 32 && !_floatTypeId) {
            _floatTypeId = result;
        }
    }

    void OpConstant(Id resultType, Id result, Literal value) override
    {
        const Word v = _SpvByteCode::LiteralAsInt(value);
        if (resultType == _intTypeId) {
            _intConstantIds.try_emplace(v, result);
        } else if (resultType == _uintTypeId) {
            _uintConstantIds.try_emplace(v, result);
        } else if (resultType == _floatTypeId) {
            _floatConstantIds.try_emplace(v, result);
        }
    }

protected:
    Id _EnsureIntType()
    {
        if (!_intTypeId) {
            _intTypeId = _byteCode.NextId();
            _byteCode.Inject(SpvOpTypeInt, {_intTypeId, 32, 1});
        }
        return _intTypeId;
    }

    Id _EnsureUintType()
    {
        if (!_uintTypeId) {
            _uintTypeId = _byteCode.NextId();
            _byteCode.Inject(SpvOpTypeInt, {_uintTypeId, 32, 0});
        }
        return _uintTypeId;
    }

    Id _EnsureFloatType()
    {
        if (!_floatTypeId) {
            _floatTypeId = _byteCode.NextId();
            _byteCode.Inject(SpvOpTypeFloat, {_floatTypeId, 32});
        }
        return _floatTypeId;
    }

    Id _EnsureBoolType()
    {
        if (!_boolTypeId) {
            _boolTypeId = _byteCode.NextId();
            _byteCode.Inject(SpvOpTypeBool, {_boolTypeId});
        }
        return _boolTypeId;
    }

    Id _EnsureConstInt(int32_t value)
    {
        const auto [it, inserted] = _intConstantIds.try_emplace(value, Id{});
        if (inserted) {
            it->second = _byteCode.NextId();
            _byteCode.Inject(SpvOpConstant,
                {_intTypeId, it->second, static_cast<Word>(value)});
        }
        return it->second;
    }

    Id _EnsureConstUint(uint32_t value)
    {
        const auto [it, inserted] = _uintConstantIds.try_emplace(value, Id{});
        if (inserted) {
            it->second = _byteCode.NextId();
            _byteCode.Inject(SpvOpConstant,
                {_uintTypeId, it->second, static_cast<Word>(value)});
        }
        return it->second;
    }

    Id _EnsureConstFloat(float value)
    {
        Word bits{};
        std::memcpy(&bits, &value, sizeof(Word));
        const auto [it, inserted] = _floatConstantIds.try_emplace(bits, Id{});
        if (inserted) {
            it->second = _byteCode.NextId();
            _byteCode.Inject(SpvOpConstant, {_floatTypeId, it->second, bits});
        }
        return it->second;
    }

    Id _GetConstInt(int32_t value, bool failIfMissing = true) const
    {
        const auto it = _intConstantIds.find(value);
        if (it == _intConstantIds.end()) {
            if (failIfMissing) {
                TF_CODING_ERROR("Constant int value not ensured: %d", value);
            }
            return 0;
        }
        return it->second;
    }

    Id _GetConstUint(uint32_t value, bool failIfMissing = true) const
    {
        const auto it = _uintConstantIds.find(value);
        if (it == _uintConstantIds.end()) {
            if (failIfMissing) {
                TF_CODING_ERROR("Constant uint value not ensured: %u", value);
            }
            return 0;
        }
        return it->second;
    }

    Id _GetConstFloat(float value, bool failIfMissing = true) const
    {
        Word bits{};
        std::memcpy(&bits, &value, sizeof(Word));
        const auto it = _floatConstantIds.find(bits);
        if (it == _floatConstantIds.end()) {
            if (failIfMissing) {
                TF_CODING_ERROR("Constant float value not ensured: %f", value);
            }
            return 0;
        }
        return it->second;
    }

    Id _boolTypeId = 0;
    Id _intTypeId = 0;
    Id _uintTypeId = 0;
    Id _floatTypeId = 0;

private:
    std::unordered_map<Word, Id> _intConstantIds;
    std::unordered_map<Word, Id> _uintConstantIds;
    std::unordered_map<Word, Id> _floatConstantIds;
};

/// Scan for interface variables required by a transform, and which needs to be
/// located before the transform pass can run. Add more as necessary.
class _FindInterfaceVariables : public _SpvVisitor
{
    using _SpvVisitor::_SpvVisitor;

protected:
    void OpDecorate(Id target, SpvDecoration decoration,
        const std::vector<Literal>& extra) override
    {
        if (extra.empty()) {
            return;
        }

        if (decoration == SpvDecorationBuiltIn) {
            const auto builtIn =
                _SpvByteCode::LiteralAsEnum<SpvBuiltIn>(extra.front());
            if (builtIn == SpvBuiltInSampleMask) {
                _sampleMaskIdCandidate = target;
            } else if (builtIn == SpvBuiltInFragCoord) {
                _fragCoordIdCandidate = target;
            }
        } else if (decoration == SpvDecorationLocation) {
            const auto location = _SpvByteCode::LiteralAsInt(extra.front());
            if (location == 0) {
                _colorOutputIdCandidates.try_emplace(target, location);
            }
        }
    }

    void OpVariable(Id, Id result, SpvStorageClass storageClass,
        std::optional<Operand>) override
    {
        if (storageClass == SpvStorageClassInput) {
            if (result == _fragCoordIdCandidate) {
                fragCoordId = result;
            }
        } else if (storageClass == SpvStorageClassOutput) {
            if (result == _sampleMaskIdCandidate) {
                sampleMaskId = result;
            }
            if (const auto iter = _colorOutputIdCandidates.find(result);
                iter != _colorOutputIdCandidates.end()) {
                colorOutputIds.try_emplace(iter->second, iter->first);
            }
        }
    }

public:
    Id sampleMaskId{};
    Id fragCoordId{};
    std::unordered_map<Word, Id> colorOutputIds;

private:
    Id _sampleMaskIdCandidate{};
    Id _fragCoordIdCandidate{};
    std::unordered_map<Id, Word> _colorOutputIdCandidates;
};

/// Negate the output vertex position Y coordinate, at the very end of the
/// shader. This is achieved by redirecting the current vertex entry point to a
/// new one, which is injected right after the original one. This new entry
/// point calls the original one, negates the output vertex position Y
/// coordinate, then returns. In GLSL, it looks like this:
///     out gl_PerVertex
///     {
///         vec4 gl_Position;
///     };
///
///     void old_main() { ... } // renamed
///
///     void main()
///     {
///         old_main();
///         gl_PerVertex.gl_Position.y = -gl_PerVertex.gl_Position.y;
///     }
///
/// In SPIR-V, gl_PerVertex is compiled to a "Block" struct. Its value is
/// exposed as an "Output" pointer variable to the struct. In pseudo C++, it
/// looks like this:
///     [[Block]] struct gl_PerVertex
///     {
///         [[BuiltIn(Position)]] vec4 gl_Position;
///     };
///     [[Output]] gl_PerVertex* vertexOut;
///
///     void old_main() { ... } // renamed
///
///     [[Interface(vertexOut)]]
///     void main()
///     {
///         old_main();
///         vertexOut->gl_Position.y = -vertexOut->gl_Position.y;
///     }
///
/// To do the transformation, we locate all structs annotated (decorated)
/// with Block and with a BuiltIn Position member. We look for an output
/// variable having such a struct pointer type, and also belonging to the entry
/// point interface variable list. Then after the original entry point has been
/// parsed, we write our new entry point, using the information we collected
/// about the variables and structs to negate gl_Position.y in the output.
///
/// Finally, in SPIR-V it looks like this (some code omitted):
///                          OpEntryPoint Vertex %main "main" %gl_PerVertex_var
///                          ; bytecode omitted...
///                          OpMemberDecorate %gl_PerVertex 0 BuiltIn Position
///                          OpDecorate %gl_PerVertex Block
///                          ; bytecode omitted...
///          %gl_PerVertex = OpTypeStruct %vec4
///  %gl_PerVertex_out_ptr = OpTypePointer Output %gl_PerVertex
///      %gl_PerVertex_var = OpVariable %gl_PerVertex_out_ptr Output
///                          ; bytecode omitted...
///                          ; \/ our injected entry point \/
///                  %main = OpFunction %void None %main_func
///                %unused = OpLabel
///           %void_return = OpFunctionCall %void %old_main
///        %position_y_ptr = OpAccessChain %out_float_ptr %gl_PerVertex_var
///                          %int_0 %int_1
///            %position_y = OpLoad %float %position_y_ptr
///      %minus_position_y = OpFNegate %float %position_y
///                          OpStore %position_y_ptr %minus_position_y
///                          OpReturn
///                          OpFunctionEnd
///
/// Note that shader interface with Block structs is defined in the Vulkan spec,
/// so we can assume GLSL is always compiled this way. See:
/// https://registry.khronos.org/vulkan/specs/1.3/html/vkspec.html#interfaces-iointerfaces
class _FlipYVisitor final : public _SpvTypeCollector
{
public:
    using _SpvTypeCollector::_SpvTypeCollector;

    ~_FlipYVisitor() override = default;

    void OpEntryPoint(SpvExecutionModel executionModel, Id entryPoint,
        std::string_view name, TfSpan<const Operand> variables) override
    {
        if (executionModel == SpvExecutionModelVertex) {
            _entryPointsById.try_emplace(entryPoint,
                _EntryPoint{_byteCode.CurrentOffset(),
                    {variables.begin(), variables.end()}});
        }
    }

    void OpDecorate(Id target, SpvDecoration decoration,
        const std::vector<Literal>& extra) override
    {
        // OpDecorate and OpMemberDecorate are un-sequenced,
        // so record all structs that are Block or with a
        // BuiltIn Position member. We'll filter those with only
        // both in OpVariable, when all decoration are complete.
        if (decoration == SpvDecorationBlock) {
            if (const auto it = _structByTypeId.find(target);
                it != _structByTypeId.end()) {
                it->second._isBlock = true;
            } else {
                _structByTypeId.try_emplace(
                    target, _Struct{std::nullopt, true});
            }
        }
    }

    void OpMemberDecorate(Id structType, Word member, SpvDecoration decoration,
        const std::vector<Literal>& extra) override
    {
        if (decoration == SpvDecorationBuiltIn &&
            _SpvByteCode::LiteralAsEnum<SpvBuiltIn>(extra.front()) ==
                SpvBuiltInPosition) {
            if (const auto it = _structByTypeId.find(structType);
                it != _structByTypeId.end()) {
                it->second._positionBuiltInMember = member;
            } else {
                _structByTypeId.try_emplace(structType, _Struct{member, false});
            }
        }
    }

    void OpTypePointer(
        Id result, SpvStorageClass storageClass, Id pointeeType) override
    {
        if (storageClass == SpvStorageClassOutput) {
            if (pointeeType == _floatTypeId) {
                _floatPointerTypeId = result;
            } else if (const auto it = _structByTypeId.find(pointeeType);
                it != _structByTypeId.end()) {
                // Variables are always pointer types, so record those.
                _structByPointerTypeId.try_emplace(result, &it->second);
            }
        }
    }

    void OpVariable(Id resultType, Id result, SpvStorageClass storageClass,
        std::optional<Operand> initializer) override
    {
        if (storageClass != SpvStorageClassOutput) {
            return;
        }

        // If a variable has a vertex output
        // block struct type, then record it.
        if (const auto it = _structByPointerTypeId.find(resultType);
            it != _structByPointerTypeId.end()) {
            if (const auto* struct_ = it->second;
                struct_->_isBlock && struct_->_positionBuiltInMember) {
                _vertexOutByVariableId.try_emplace(
                    result, _VertexOutStruct{*struct_->_positionBuiltInMember});
            }
        }
    }

    void BeginFunctions() override
    {
        // Write out any missing types and constants
        // just before the first OpFunction, as required
        // by the SPIR-V module logical layout.

        _EnsureIntType();
        const Id floatTypeId = _EnsureFloatType();
        if (!_floatPointerTypeId) {
            _floatPointerTypeId = _byteCode.NextId();
            _byteCode.Inject(SpvOpTypePointer,
                {_floatPointerTypeId, SpvStorageClassOutput, floatTypeId});
        }

        _EnsureConstInt(1u); // y component index

        for (const auto& [_, vertexOut] : _vertexOutByVariableId) {
            _EnsureConstInt(vertexOut._positionBuiltInMember);
        }
    }

    void OpFunction(Id resultType, Id result,
        SpvFunctionControlMask functionControl, Id functionType) override
    {
        // Look for the vertex entry points to override,
        // by using the interface variable list.
        if (const auto entryIt = _entryPointsById.find(result);
            entryIt != _entryPointsById.end()) {
            for (Id variable : entryIt->second._interfaceVariables) {
                if (const auto structIt = _vertexOutByVariableId.find(variable);
                    structIt != _vertexOutByVariableId.end()) {
                    // Record all the data we need to define
                    // the new entry point and call the old one.
                    entryIt->second._functionId = result;
                    entryIt->second._functionTypeId = functionType;
                    entryIt->second._resultTypeId = resultType;
                    entryIt->second._outputVariableId = variable;
                    entryIt->second._vertexOut = &structIt->second;
                    // It's a declaration unless we
                    // see a label before OpFunctionEnd
                    _currentEntryPointDeclaration = &entryIt->second;
                    break;
                }
            }
        }
    }

    void OpFunctionEnd() override
    {
        if (!_currentEntryPointDefinition) {
            return;
        }

        // See the class documentation for an explanation of this code.
        const Id newEntryPointId = _byteCode.NextId();
        _byteCode.Modify(_currentEntryPointDefinition->_instructionOffset + 2,
            newEntryPointId);

        _byteCode.Inject(SpvOpFunction,
            {_currentEntryPointDefinition->_resultTypeId, newEntryPointId,
                SpvFunctionControlMaskNone,
                _currentEntryPointDefinition->_functionTypeId});
        _byteCode.Inject(SpvOpLabel, {_byteCode.NextId()});

        _byteCode.Inject(SpvOpFunctionCall,
            {_currentEntryPointDefinition->_resultTypeId, _byteCode.NextId(),
                _currentEntryPointDefinition->_functionId});

        const Id positionYPtr = _byteCode.NextId();
        _byteCode.Inject(SpvOpAccessChain,
            {_floatPointerTypeId, positionYPtr,
                _currentEntryPointDefinition->_outputVariableId,
                _GetConstInt(_currentEntryPointDefinition->_vertexOut
                        ->_positionBuiltInMember),
                _GetConstInt(1)});
        const Id positionY = _byteCode.NextId();
        _byteCode.Inject(SpvOpLoad, {_floatTypeId, positionY, positionYPtr});
        const Id minusPositionY = _byteCode.NextId();
        _byteCode.Inject(
            SpvOpFNegate, {_floatTypeId, minusPositionY, positionY});
        _byteCode.Inject(SpvOpStore, {positionYPtr, minusPositionY});

        _byteCode.Inject(SpvOpReturn, {});
        _byteCode.Inject(SpvOpFunctionEnd, {});

        _currentEntryPointDefinition = nullptr;
    }

    void OpLabel(Id result) override
    {
        if (_currentEntryPointDeclaration) {
            // Label marks this as a definition
            _currentEntryPointDefinition = _currentEntryPointDeclaration;
            _currentEntryPointDeclaration = nullptr;
        }
    }

private:
    /// Vertex output struct candidates
    struct _Struct
    {
        std::optional<Word> _positionBuiltInMember;
        bool _isBlock = false;
    };
    std::unordered_map<Id, _Struct> _structByTypeId;
    std::unordered_map<Id, _Struct*> _structByPointerTypeId;

    /// A vertex output struct
    struct _VertexOutStruct
    {
        // Initialize with an invalid value
        Word _positionBuiltInMember = std::numeric_limits<Word>::max();
    };
    std::unordered_map<Id, _VertexOutStruct> _vertexOutByVariableId;

    /// A vertex entry point to redirect
    struct _EntryPoint
    {
        size_t _instructionOffset = 0;
        std::unordered_set<Id> _interfaceVariables;
        Id _functionId = 0;
        Id _functionTypeId = 0;
        Id _resultTypeId = 0;
        Id _outputVariableId = 0;
        _VertexOutStruct* _vertexOut = nullptr;
    };
    std::unordered_map<Id, _EntryPoint> _entryPointsById;

    Id _floatPointerTypeId = 0;

    /// Track the entry point we're currently visiting
    _EntryPoint* _currentEntryPointDeclaration = nullptr;
    _EntryPoint* _currentEntryPointDefinition = nullptr;
};

/// WebGPU has no equivalent of GL_SAMPLE_ALPHA_TO_ONE. Without it, coverage
/// alpha is also written to the render target, making it transparent. Emulate
/// GL_SAMPLE_ALPHA_TO_ONE for WebGPU fragment shaders by redirecting the
/// fragment entry point to a new wrapper, injected immediately after the
/// original. The wrapper calls the original entry point, computes a coverage
/// mask from the fragment alpha and pixel coordinates, then stores it to a new
/// SampleMask output and overwrites alpha with 1.
///
/// Alpha-to-coverage must be disabled when this transform is used, since sample
/// coverage is driven by the shader's sample mask output instead.
///
/// In GLSL, the injected wrapper looks like this (with sampleCount = 4):
///     layout(location = 0) out vec4 color; // must exist
///     in vec4 gl_FragCoord; // injected if absent
///     out int gl_SampleMask[1]; // injected if absent
///
///     void old_main() { ... }  // renamed
///
///     void main()
///     {
///         gl_SampleMask[0] = ~0;
///         old_main();
///         // 2x2 Bayer ordered dithering (matches hardware alpha-to-coverage)
///         uint ditherIndex = (uint(gl_FragCoord.x) & 1u) |
///             ((uint(gl_FragCoord.y) & 1u) << 1u);
///         float threshold = vec4(0.125, 0.625, 0.875, 0.375)[ditherIndex] /
///             sampleCount;
///         uint coverage = uint(clamp((color.a - threshold) *
///             sampleCount + 1.0, 0.0, sampleCount));
///         gl_SampleMask[0] &= int((1u << coverage) - 1u);
///         color.a = 1;
///     }
///
/// To do the transformation, we find the location = 0 color output via
/// OpDecorate, inject a FragCoord input variable and SampleMask output variable
/// (unless already present). After the original entry point is parsed, we write
/// the wrapper function.
///
/// Finally, in SPIR-V it looks like this (some code omitted):
///                        %glslInstructions = OpExtInstImport "GLSL.std.450"
///                        ; bytecode omitted...
///                        OpEntryPoint Fragment %main "main" %gl_FragCoord
///                        %color %gl_SampleMask
///                        OpExecutionMode %main OriginUpperLeft
///                        OpDecorate %gl_FragCoord BuiltIn FragCoord
///                        OpDecorate %gl_SampleMask BuiltIn SampleMask
///                        OpDecorate %color Location 0
///                        ; bytecode omitted...
///          %int_array1 = OpTypeArray %int %int_1
///  %int_array1_out_ptr = OpTypePointer Output %int_array1
///       %gl_SampleMask = OpVariable %int_array1_out_ptr Output
///                        ; bytecode omitted...
///                        ; \/ our injected entry point \/
///                %main = OpFunction %void None %4
///              %unused = OpLabel
///          %frag_x_ptr = OpAccessChain %float_in_ptr %gl_FragCoord %int_0
///              %frag_x = OpLoad %float %frag_x_ptr
///      %frag_x_floored = OpConvertFToS %int %frag_x
///         %frag_x_even = OpBitwiseAnd %int %frag_x_floored %int_1
///          %frag_y_ptr = OpAccessChain %float_in_ptr %gl_FragCoord %int_1
///              %frag_y = OpLoad %float %frag_y_ptr
///      %frag_y_floored = OpConvertFToS %int %frag_y
///         %frag_y_even = OpBitwiseAnd %int %frag_y_floored %int_1
///        %frag_y_even2 = OpShiftLeftLogical %int %frag_y_even %int_1
///         %bayer_index = OpBitwiseOr %int %frag_x_even %frag_y_even2
///           %threshold = OpVectorExtractDynamic %float %bayer_matrix
///                        %bayer_index
///           %alpha_ptr = OpAccessChain %float_out_ptr %color %int_3
///               %alpha = OpLoad %float %alpha_ptr
///      %dithered_alpha = OpFSub %float %alpha %threshold
///        %scaled_alpha = OpFMul %float %dithered_alpha %float_4
/// %scaled_biased_alpha = OpFAdd %float %scaled_alpha %float_1
///       %clamped_alpha = OpExtInst %float %glslInstructions FClamp
///                        %scaled_biased_alpha %float_0 %float_4
///           %bit_count = OpConvertFToS %int %clamped_alpha
///        %bit_mask_tmp = OpShiftLeftLogical %int %int_1 %bit_count
///            %bit_mask = OpISub %int %bit_mask_tmp %int_1
///       %coverage_mask = OpBitcast %int %bit_mask
/// %sample_mask_out_ptr = OpAccessChain %int_out_ptr %gl_SampleMask %int_0
///         %sample_mask = OpLoad %int %sample_mask_out_ptr
///     %new_sample_mask = OpBitwiseAnd %int %sample_mask %coverage_mask
///                        OpStore %sample_mask_out_ptr %new_sample_mask
///                        OpStore %alpha_ptr %float_1
///                        OpReturn
///                        OpFunctionEnd
class _AlphaToOneVisitor final : public _SpvTypeCollector
{
public:
    _AlphaToOneVisitor(_SpvByteCode& byteCode, uint32_t sampleCount,
        const _FindInterfaceVariables& interfaceVariables)
        : _SpvTypeCollector{byteCode}
        , _sampleCount{static_cast<float>(sampleCount)}
        , _interfaceVariables(interfaceVariables)
    {
        const std::array<float, 4> bayerValues{0.125f, 0.625f, 0.875f, 0.375f};
        for (int i = 0; i < 4; i++) {
            _bayerMatrix[i] = bayerValues[i] / _sampleCount;
        }
    }

    ~_AlphaToOneVisitor() override = default;

    void OpExtInstImport(Id result, std::string_view name) override
    {
        if (!_glslInstructionSet && name == "GLSL.std.450") {
            _glslInstructionSet = result;
        }
    }

    void OpEntryPoint(SpvExecutionModel executionModel, Id entryPoint,
        std::string_view, TfSpan<const Operand> variables) override
    {
        if (executionModel != SpvExecutionModelFragment) {
            return;
        }

        // Alpha-to-coverage only applies to the first color output
        if (_interfaceVariables.colorOutputIds.empty() ||
            std::find(variables.begin(), variables.end(),
                _interfaceVariables.colorOutputIds.at(0)) == variables.end()) {
            return;
        }

        _entryPointsById.try_emplace(
            entryPoint, _EntryPoint{_byteCode.CurrentOffset()});

        _sampleMaskId = _interfaceVariables.sampleMaskId;
        if (!_sampleMaskId) {
            _sampleMaskId = _byteCode.NextId();
        }
        if (std::find(variables.begin(), variables.end(), _sampleMaskId) ==
            variables.end()) {
            _byteCode.InsertWord(_sampleMaskId);
        }

        _fragCoordId = _interfaceVariables.fragCoordId;
        if (!_fragCoordId) {
            _fragCoordId = _byteCode.NextId();
        }
        if (std::find(variables.begin(), variables.end(), _fragCoordId) ==
            variables.end()) {
            _byteCode.InsertWord(_fragCoordId);
        }
    }

    void OpExecutionMode(Id entryPoint, SpvExecutionMode executionMode,
        const std::vector<Literal>& extra) override
    {
        if (const auto iter = _entryPointsById.find(entryPoint);
            iter != _entryPointsById.end()) {
            iter->second._executionModeInstructionOffsets.push_back(
                _byteCode.CurrentOffset());
        }
    }

    void OpDecorate(Id target, SpvDecoration decoration,
        const std::vector<Literal>& extra) override
    {
        if (!_decorationsInjected) {
            if (!_interfaceVariables.sampleMaskId && _sampleMaskId) {
                _byteCode.Inject(SpvOpDecorate,
                    {_sampleMaskId, static_cast<Word>(SpvDecorationBuiltIn),
                        static_cast<Word>(SpvBuiltInSampleMask)});
            }
            if (!_interfaceVariables.fragCoordId && _fragCoordId) {
                _byteCode.Inject(SpvOpDecorate,
                    {_fragCoordId, static_cast<Word>(SpvDecorationBuiltIn),
                        static_cast<Word>(SpvBuiltInFragCoord)});
            }
            _decorationsInjected = true;
        }
    }

    void OpTypeVector(Id result, Id componentType, Word count) override
    {
        if (!_vec4fTypeId && componentType == _floatTypeId && count == 4) {
            _vec4fTypeId = result;
        }
    }

    void OpTypeArray(Id result, Id componentType, Id length) override
    {
        if (!_intArray1TypeId && componentType == _intTypeId &&
            (length == _GetConstInt(1, false) ||
                length == _GetConstUint(1u, false))) {
            _intArray1TypeId = result;
        }
    }

    void OpTypePointer(
        Id result, SpvStorageClass storageClass, Id pointeeType) override
    {
        if (storageClass == SpvStorageClassOutput) {
            if (pointeeType == _floatTypeId && !_floatOutputPtrTypeId) {
                _floatOutputPtrTypeId = result;
            } else if (pointeeType == _intTypeId && !_intOutputPtrTypeId) {
                _intOutputPtrTypeId = result;
            } else if (pointeeType == _intArray1TypeId &&
                !_intArray1OutputPtrTypeId) {
                _intArray1OutputPtrTypeId = result;
            }
        } else if (storageClass == SpvStorageClassInput) {
            if (pointeeType == _vec4fTypeId && !_vec4fInputPtrTypeId) {
                _vec4fInputPtrTypeId = result;
            } else if (pointeeType == _floatTypeId && !_floatInputPtrTypeId) {
                _floatInputPtrTypeId = result;
            }
        }
    }

    void BeginFunctions() override
    {
        const Id floatTypeId = _EnsureFloatType();
        const Id intTypeId = _EnsureIntType();

        _EnsureConstInt(0);
        _EnsureConstInt(1);
        _EnsureConstInt(3);
        _EnsureConstInt(~0);
        
        _EnsureConstFloat(0);
        _EnsureConstFloat(1);
        _EnsureConstFloat(_sampleCount);

        _bayerMatrixId = _byteCode.NextId();
        _byteCode.Inject(SpvOpConstantComposite,
            {
                _vec4fTypeId,
                _bayerMatrixId,
                _EnsureConstFloat(_bayerMatrix[0]),
                _EnsureConstFloat(_bayerMatrix[1]),
                _EnsureConstFloat(_bayerMatrix[2]),
                _EnsureConstFloat(_bayerMatrix[3]),
            });

        if (!_floatOutputPtrTypeId) {
            _floatOutputPtrTypeId = _byteCode.NextId();
            _byteCode.Inject(SpvOpTypePointer,
                {_floatOutputPtrTypeId, SpvStorageClassOutput, floatTypeId});
        }

        if (!_intArray1TypeId) {
            _intArray1TypeId = _byteCode.NextId();
            _byteCode.Inject(SpvOpTypeArray,
                {_intArray1TypeId, intTypeId, _EnsureConstInt(1u)});
        }

        if (!_intOutputPtrTypeId) {
            _intOutputPtrTypeId = _byteCode.NextId();
            _byteCode.Inject(SpvOpTypePointer,
                {_intOutputPtrTypeId, SpvStorageClassOutput, intTypeId});
        }

        if (!_floatInputPtrTypeId) {
            _floatInputPtrTypeId = _byteCode.NextId();
            _byteCode.Inject(SpvOpTypePointer,
                {_floatInputPtrTypeId, SpvStorageClassInput, floatTypeId});
        }

        if (!_vec4fTypeId) {
            _vec4fTypeId = _byteCode.NextId();
            _byteCode.Inject(SpvOpTypeVector, {_vec4fTypeId, _floatTypeId, 4u});
        }

        if (!_interfaceVariables.sampleMaskId && _sampleMaskId) {
            if (!_intArray1OutputPtrTypeId) {
                _intArray1OutputPtrTypeId = _byteCode.NextId();
                _byteCode.Inject(SpvOpTypePointer,
                    {_intArray1OutputPtrTypeId, SpvStorageClassOutput,
                        _intArray1TypeId});
            }
            _byteCode.Inject(SpvOpVariable,
                {_intArray1OutputPtrTypeId, _sampleMaskId,
                    SpvStorageClassOutput});
        }

        if (!_interfaceVariables.fragCoordId && _fragCoordId) {
            if (!_vec4fInputPtrTypeId) {
                _vec4fInputPtrTypeId = _byteCode.NextId();
                _byteCode.Inject(SpvOpTypePointer,
                    {_vec4fInputPtrTypeId, SpvStorageClassInput, _vec4fTypeId});
            }
            _byteCode.Inject(SpvOpVariable,
                {_vec4fInputPtrTypeId, _fragCoordId, SpvStorageClassInput});
        }
    }

    void OpFunction(Id resultType, Id result, SpvFunctionControlMask,
        Id functionType) override
    {
        if (const auto entryIt = _entryPointsById.find(result);
            entryIt != _entryPointsById.end()) {
            entryIt->second._functionId = result;
            entryIt->second._functionTypeId = functionType;
            entryIt->second._resultTypeId = resultType;
            _currentEntryPointDeclaration = &entryIt->second;
        }
    }

    void OpFunctionEnd() override
    {
        if (!_currentEntryPointDefinition) {
            return;
        }

        const Id int0 = _GetConstInt(0);
        const Id int1 = _GetConstInt(1);
        const Id float0 = _GetConstFloat(0);
        const Id float1 = _GetConstFloat(1);

        // Redirect the entry point to our new function
        const Id newEntryPointId = _byteCode.NextId();
        _byteCode.Modify(_currentEntryPointDefinition->_instructionOffset + 2,
            newEntryPointId);
        for (const size_t execModeOffset :
            _currentEntryPointDefinition->_executionModeInstructionOffsets) {
            _byteCode.Modify(execModeOffset + 1, newEntryPointId);
        }

        // Start the new entry point function
        _byteCode.Inject(SpvOpFunction,
            {_currentEntryPointDefinition->_resultTypeId, newEntryPointId,
                SpvFunctionControlMaskNone,
                _currentEntryPointDefinition->_functionTypeId});
        _byteCode.Inject(SpvOpLabel, {_byteCode.NextId()});

        // Initialize gl_SampleMask to all 1s
        const Id sampleMaskPtr = _byteCode.NextId();
        _byteCode.Inject(SpvOpAccessChain,
            {_intOutputPtrTypeId, sampleMaskPtr, _sampleMaskId, int0});
        _byteCode.Inject(SpvOpStore, {sampleMaskPtr, _GetConstInt(~0)});

        _byteCode.Inject(SpvOpFunctionCall,
            {_currentEntryPointDefinition->_resultTypeId, _byteCode.NextId(),
                _currentEntryPointDefinition->_functionId});

        // Get gl_FragCoord.x % 2
        const Id xFragPtr = _byteCode.NextId();
        _byteCode.Inject(SpvOpAccessChain,
            {_floatInputPtrTypeId, xFragPtr, _fragCoordId, int0});
        const Id xFrag = _byteCode.NextId();
        _byteCode.Inject(SpvOpLoad, {_floatTypeId, xFrag, xFragPtr});
        const Id xFragFloor = _byteCode.NextId();
        _byteCode.Inject(SpvOpConvertFToS, {_intTypeId, xFragFloor, xFrag});
        const Id xFragEven = _byteCode.NextId();
        _byteCode.Inject(
            SpvOpBitwiseAnd, {_intTypeId, xFragEven, xFragFloor, int1});

        // Get gl_FragCoord.y % 2
        const Id yFragPtr = _byteCode.NextId();
        _byteCode.Inject(SpvOpAccessChain,
            {_floatInputPtrTypeId, yFragPtr, _fragCoordId, int1});
        const Id yFrag = _byteCode.NextId();
        _byteCode.Inject(SpvOpLoad, {_floatTypeId, yFrag, yFragPtr});
        const Id yFragFloor = _byteCode.NextId();
        _byteCode.Inject(SpvOpConvertFToS, {_intTypeId, yFragFloor, yFrag});
        const Id yFragEven = _byteCode.NextId();
        _byteCode.Inject(
            SpvOpBitwiseAnd, {_intTypeId, yFragEven, yFragFloor, int1});

        // Compute the 2x2 bayer matrix index from the mod 2 fragment
        // coordinates, and get the dither threshold value.
        const Id yFragEven2 = _byteCode.NextId();
        _byteCode.Inject(
            SpvOpShiftLeftLogical, {_intTypeId, yFragEven2, yFragEven, int1});
        const Id bayerIndex = _byteCode.NextId();
        _byteCode.Inject(
            SpvOpBitwiseOr, {_intTypeId, bayerIndex, xFragEven, yFragEven2});
        const Id threshold = _byteCode.NextId();
        _byteCode.Inject(SpvOpVectorExtractDynamic,
            {_floatTypeId, threshold, _bayerMatrixId, bayerIndex});

        // Apply the dither threshold to the alpha value and convert to a
        // coverage count.
        const Id alphaPtr = _byteCode.NextId();
        _byteCode.Inject(SpvOpAccessChain,
            {_floatOutputPtrTypeId, alphaPtr,
                _interfaceVariables.colorOutputIds.at(0), _GetConstInt(3)});
        const Id alpha = _byteCode.NextId();
        _byteCode.Inject(SpvOpLoad, {_floatTypeId, alpha, alphaPtr});
        const Id ditheredAlpha = _byteCode.NextId();
        _byteCode.Inject(
            SpvOpFSub, {_floatTypeId, ditheredAlpha, alpha, threshold});
        const Id floatSampleCount = _GetConstFloat(_sampleCount);
        const Id scaledAlpha = _byteCode.NextId();
        _byteCode.Inject(SpvOpFMul,
            {_floatTypeId, scaledAlpha, ditheredAlpha, floatSampleCount});
        const Id scaledBiasedAlpha = _byteCode.NextId();
        _byteCode.Inject(
            SpvOpFAdd, {_floatTypeId, scaledBiasedAlpha, scaledAlpha, float1});
        const Id clampedAlpha = _byteCode.NextId();
        _byteCode.Inject(SpvOpExtInst,
            {_floatTypeId, clampedAlpha, _glslInstructionSet, GLSLstd450FClamp,
                scaledBiasedAlpha, float0, floatSampleCount});

        // Convert that coverage count to a coverage mask.
        const Id coveredCount = _byteCode.NextId();
        _byteCode.Inject(
            SpvOpConvertFToS, {_intTypeId, coveredCount, clampedAlpha});
        const Id coverageTmp = _byteCode.NextId();
        _byteCode.Inject(SpvOpShiftLeftLogical,
            {_intTypeId, coverageTmp, int1, coveredCount});
        const Id coverageMask = _byteCode.NextId();
        _byteCode.Inject(
            SpvOpISub, {_intTypeId, coverageMask, coverageTmp, int1});

        // Bitwise-and that coverage mask with the one written by the old entry
        // point (if it's written, that's why we initialialize it).
        const Id sampleMask = _byteCode.NextId();
        _byteCode.Inject(SpvOpLoad, {_intTypeId, sampleMask, sampleMaskPtr});
        const Id newSampleMask = _byteCode.NextId();
        _byteCode.Inject(SpvOpBitwiseAnd,
            {_intTypeId, newSampleMask, sampleMask, coverageMask});

        // Write the new sample mask and reset alpha to 1.
        _byteCode.Inject(SpvOpStore, {sampleMaskPtr, newSampleMask});
        _byteCode.Inject(SpvOpStore, {alphaPtr, float1});

        // End new entry point function.
        _byteCode.Inject(SpvOpReturn, {});
        _byteCode.Inject(SpvOpFunctionEnd, {});

        _currentEntryPointDefinition = nullptr;
    }

    void OpLabel(Id label) override
    {
        if (_currentEntryPointDeclaration) {
            // Label marks this as a definition
            _currentEntryPointDefinition = _currentEntryPointDeclaration;
            _currentEntryPointDeclaration = nullptr;
        }
    }

private:
    float _sampleCount = 4;
    std::array<float, 4> _bayerMatrix{};

    const _FindInterfaceVariables& _interfaceVariables;
    Id _sampleMaskId{};
    Id _fragCoordId{};
    bool _decorationsInjected = false;

    Id _glslInstructionSet = 0;

    Id _vec4fTypeId = 0;
    Id _intArray1TypeId = 0;
    Id _floatOutputPtrTypeId = 0;
    Id _intOutputPtrTypeId = 0;
    Id _vec4fInputPtrTypeId = 0;
    Id _floatInputPtrTypeId = 0;
    Id _intArray1OutputPtrTypeId = 0;

    Id _bayerMatrixId = 0;

    struct _EntryPoint
    {
        size_t _instructionOffset = 0;
        std::vector<size_t> _executionModeInstructionOffsets;
        Id _functionId = 0;
        Id _functionTypeId = 0;
        Id _resultTypeId = 0;
    };
    std::unordered_map<Id, _EntryPoint> _entryPointsById;

    /// Track the entry point we're currently visiting
    _EntryPoint* _currentEntryPointDeclaration = nullptr;
    _EntryPoint* _currentEntryPointDefinition = nullptr;
};

bool
ApplySpirvViewportFlip(std::vector<uint32_t>& spirv)
{
    TRACE_FUNCTION();

    _SpvByteCode byteCode{spirv};
    if (ARCH_UNLIKELY(!byteCode.Apply<_FlipYVisitor>())) {
        return false;
    }

    spirv = byteCode.TakeResult();
    return true;
}

bool
ApplySpirvAlphaToOneEmulation(
    std::vector<uint32_t>& spirv, uint32_t sampleCount)
{
    TRACE_FUNCTION();

    _SpvByteCode byteCode{spirv};

    _FindInterfaceVariables interface{byteCode};
    if (ARCH_UNLIKELY(!byteCode.Apply(interface))) {
        return false;
    }

    // No applicable alpha output for alpha-to-coverage,
    // so alpha is always 1 and coverage is unaffected.
    if (!interface.colorOutputIds.count(0)) {
        return true;
    }

    _AlphaToOneVisitor visitor{byteCode, sampleCount, interface};
    if (ARCH_UNLIKELY(!byteCode.Apply(visitor))) {
        return false;
    }

    spirv = byteCode.TakeResult();
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
