//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/variableExpressionParser.h"

#include "pxr/usd/sdf/debugCodes.h"
#include "pxr/usd/sdf/variableExpressionAST.h"
#include "pxr/usd/sdf/variableExpressionImpl.h"

#include "pxr/base/pegtl/pegtl.hpp"
#include "pxr/base/pegtl/pegtl/contrib/trace.hpp"
#include "pxr/base/tf/stringUtils.h"

#include <array>
#include <tuple>

using namespace PXR_PEGTL_NAMESPACE;

PXR_NAMESPACE_OPEN_SCOPE

namespace SdfVariableExpressionASTNodes
{

// Helper for calling private constructors on the various
// SdfVariableExpressionASTNodes::Node subclasses.
class _NodeCreator
{
public:
    template <class NodeType, class... Args>
    static auto MakeNode(Args&&... nodeArgs)
    {
        return std::unique_ptr<NodeType>(
            new NodeType(std::forward<Args>(nodeArgs)...));
    }
};

} // end namespace SdfVariableExpressionASTNodes

namespace
{

namespace Impl = Sdf_VariableExpressionImpl;
namespace ASTNodes = SdfVariableExpressionASTNodes;

// Node creators -----------------------------------------------
// These objects are responsible for saving intermediate values as
// an expression is being parsed and using them to create expression
// nodes.

class NodeCreator
{
public:
    virtual ~NodeCreator();
    virtual std::unique_ptr<Impl::Node> CreateNode(std::string* errMsg) = 0;
    virtual std::unique_ptr<ASTNodes::Node> CreateASTNode(
        std::string* errMsg) = 0;
};

NodeCreator::~NodeCreator() = default;

static
std::vector<std::unique_ptr<Impl::Node>>
_CreateNodes(
    const std::vector<std::unique_ptr<NodeCreator>>& creators,
    std::string* errMsg)
{
    std::vector<std::unique_ptr<Impl::Node>> nodes;
    nodes.reserve(creators.size());
    for (auto& c : creators) {
        nodes.push_back(c->CreateNode(errMsg));
    }

    return nodes;
}

static
ASTNodes::NodeList
_CreateASTNodeList(
    const std::vector<std::unique_ptr<NodeCreator>>& creators,
    std::string* errMsg)
{
    std::vector<std::unique_ptr<ASTNodes::Node>> nodes;
    nodes.reserve(creators.size());
    for (auto& c : creators) {
        nodes.push_back(c->CreateASTNode(errMsg));
    }
    return ASTNodes::NodeList(std::move(nodes));
}

class StringNodeCreator
    : public NodeCreator
{
public:
    std::unique_ptr<Impl::Node> CreateNode(std::string* errMsg) override
    {
        return std::make_unique<Impl::StringNode>(std::move(parts));
    }

    std::unique_ptr<ASTNodes::Node>
    CreateASTNode(std::string* errMsg) override
    {
        std::string s;
        for (const auto& part : parts) {
            s += part.content;
        }

        return ASTNodes::_NodeCreator::MakeNode<ASTNodes::LiteralNode>(
            std::move(s));
    }
    
    using Part = Impl::StringNode::Part;
    std::vector<Part> parts;
};

class VariableNodeCreator
    : public NodeCreator
{
public:
    std::unique_ptr<Impl::Node> CreateNode(std::string* errMsg) override
    {
        return std::make_unique<Impl::VariableNode>(std::move(var));
    }

    std::unique_ptr<ASTNodes::Node>
    CreateASTNode(std::string* errMsg) override
    {
        return ASTNodes::_NodeCreator::MakeNode<ASTNodes::VariableNode>(
            std::move(var));
    }
    
    std::string var;
};

template <class Type>
class ConstantNodeCreator
    : public NodeCreator
{
public:
    std::unique_ptr<Impl::Node> CreateNode(std::string* errMsg) override
    {
        return std::make_unique<Impl::ConstantNode<Type>>(value);
    }

    std::unique_ptr<ASTNodes::Node>
    CreateASTNode(std::string* errMsg) override
    {
        return ASTNodes::_NodeCreator::MakeNode<ASTNodes::LiteralNode>(value);
    }
    
    Type value;
};

using IntegerNodeCreator = ConstantNodeCreator<int64_t>;
using BoolNodeCreator = ConstantNodeCreator<bool>;

class NoneNodeCreator
    : public NodeCreator
{
public:
    std::unique_ptr<Impl::Node> CreateNode(std::string* errMsg) override
    {
        return std::make_unique<Impl::NoneNode>();
    }

    std::unique_ptr<ASTNodes::Node>
    CreateASTNode(std::string* errMsg) override
    {
        return ASTNodes::_NodeCreator::MakeNode<ASTNodes::LiteralNode>();
    }
};

class ListNodeCreator
    : public NodeCreator
{
public:
    std::unique_ptr<Impl::Node> CreateNode(std::string* errMsg) final
    {
        std::vector<std::unique_ptr<Impl::Node>> elemNodes = 
            _CreateNodes(elements, errMsg);
        if (!errMsg->empty()) {
            return nullptr;
        }

        return std::make_unique<Impl::ListNode>(std::move(elemNodes));
    };

    std::unique_ptr<ASTNodes::Node>
    CreateASTNode(std::string* errMsg) override
    {
        ASTNodes::NodeList astNodes = _CreateASTNodeList(elements, errMsg);
        if (!errMsg->empty()) {
            return nullptr;
        }

        return ASTNodes::_NodeCreator::MakeNode<ASTNodes::ListNode>(
            std::move(astNodes));
    }

    std::vector<std::unique_ptr<NodeCreator>> elements;
};

// List that defines the set of functions recognized by the expression parser.
// To enable a new function, add the associated function node to this list.
using AvailableFunctions = std::tuple<
    Impl::If2Node,
    Impl::If3Node,

    Impl::EqualNode,
    Impl::NotEqualNode,
    Impl::LessNode,
    Impl::LessEqualNode,
    Impl::GreaterNode,
    Impl::GreaterEqualNode,
    Impl::LogicalAndNode,
    Impl::LogicalOrNode,
    Impl::LogicalNotNode,

    Impl::ContainsNode,
    Impl::MatchesRegexNode,
    Impl::AtNode,
    Impl::LenNode,

    Impl::DefinedNode
>;

class FunctionNodeCreator
    : public NodeCreator
{
public:
    FunctionNodeCreator(const std::string& functionName_)
        : functionName(functionName_)
    { };

    std::unique_ptr<Impl::Node> CreateNode(std::string* errMsg) final
    {
        bool matchedFunctionName = false;
        return _CreateNode(
            errMsg, &matchedFunctionName, (AvailableFunctions*)(nullptr));
    }

    std::unique_ptr<ASTNodes::Node>
    CreateASTNode(std::string* errMsg) final
    {
        ASTNodes::NodeList astNodes = _CreateASTNodeList(functionArgs, errMsg);
        if (!errMsg->empty()) {
            return nullptr;
        }

        return ASTNodes::_NodeCreator::MakeNode<ASTNodes::FunctionNode>(
            std::move(functionName), std::move(astNodes));
    }

    std::string functionName;
    std::vector<std::unique_ptr<NodeCreator>> functionArgs;

private:
    // Search the list of available function nodes for one whose name matches
    // the name we received from the parser, then try to construct an instance
    // of that node with the arguments we parsed.
    template <class NodeType, class... Others>
    std::unique_ptr<Impl::Node>
    _CreateNode(
        std::string* errMsg, bool* matchedFunctionName,
        std::tuple<NodeType, Others...>*)
    {
        if (functionName == NodeType::GetFunctionName()) {
            *matchedFunctionName = true;

            std::unique_ptr<Impl::Node> node = 
                _CreateNodeHelper<NodeType>(errMsg);

            // Return if we successfully created the node or tried to do so
            // but got an error. Otherwise, we may have gotten a node with
            // the right name but the wrong number of arguments, so we need
            // to keep looking through our available function nodes.
            if (node || !errMsg->empty()) {
                return node;
            }
        }
        return _CreateNode(
            errMsg, matchedFunctionName, (std::tuple<Others...>*)(nullptr));
    }

    std::unique_ptr<Impl::Node>
    _CreateNode(
        std::string* errMsg, bool* matchedFunctionName, std::tuple<>*)
    {
        if (*matchedFunctionName) {
            *errMsg = TfStringPrintf(
                "Function '%s' does not take %zu arguments.", 
                functionName.c_str(), functionArgs.size());
        }
        else {
            *errMsg = TfStringPrintf(
                "Unknown function %s", functionName.c_str());
        }
        return nullptr;
    }

    // Construct an instance of NodeType if it's a variadic function, i.e.
    // it accepts any number of arguments beyond a given minimum.
    template <class NodeType>
    std::enable_if_t<NodeType::IsVariadic, std::unique_ptr<Impl::Node>>
    _CreateNodeHelper(std::string* errMsg)
    {
        if (functionArgs.size() < NodeType::MinNumArgs) {
            *errMsg = TfStringPrintf(
                "Function '%s' requires at least %zu arguments.",
                functionName.c_str(), NodeType::MinNumArgs);
            return nullptr;
        }

        std::vector<std::unique_ptr<Impl::Node>> argNodes =
            _CreateNodes(functionArgs, errMsg);
        if (!errMsg->empty()) {
            return nullptr;
        }
        

        return std::make_unique<NodeType>(std::move(argNodes));
    }

    // Construct an instance of NodeType if it's a function that requires a
    // specific number of arguments. Note that functions may have overloads
    // that accept different number of arguments.
    template <class NodeType>
    std::enable_if_t<!NodeType::IsVariadic, std::unique_ptr<Impl::Node>>
    _CreateNodeHelper(std::string* errMsg)
    {
        if (functionArgs.size() != NodeType::NumArgs) {
            return nullptr;
        }
        return _CreateFunctionNode<NodeType>(
            errMsg, std::make_index_sequence<NodeType::NumArgs>());
    }

    template <class NodeType, size_t... I>
    std::unique_ptr<Impl::Node> _CreateFunctionNode(
        std::string* errMsg, std::index_sequence<I...>)
    { 
        std::array<std::unique_ptr<Impl::Node>, sizeof...(I)> argNodes = {
            functionArgs[I]->CreateNode(errMsg)...
        };

        if (!errMsg->empty()) {
            return nullptr;
        }

        return std::make_unique<NodeType>(std::move(argNodes[I])...);
    }
};

// Parser state -----------------------------------------------
// Objects responsible for keeping track of intermediate state as an
// expression is being parsed.

class ParserContext
{
public:
    ParserContext() = default;
    ParserContext(const ParserContext&) = delete;
    ParserContext(ParserContext&&) = default;

    ParserContext& operator=(const ParserContext&) = delete;
    ParserContext& operator=(ParserContext&&) = default;

    // Create and push a node creator of type CreatorType onto the stack,
    // passing in any given args to CreatorType c'tor, and return it.
    template <class CreatorType, class... Args>
    CreatorType* PushNodeCreator(const Args&... args)
    {
        _nodeStack.push_back(std::make_unique<CreatorType>(args...));
        return static_cast<CreatorType*>(_nodeStack.back().get());
    }

    std::unique_ptr<NodeCreator> PopNodeCreator()
    {
        if (!TF_VERIFY(!_nodeStack.empty()) || !TF_VERIFY(_nodeStack.back())) {
            return nullptr;
        }

        std::unique_ptr<NodeCreator> creator = std::move(_nodeStack.back());
        _nodeStack.pop_back();
        return creator;
    }

    // Return pointer to the node creator on the top of the stack if
    // it is an instance of CreatorType, otherwise return nullptr.
    template <class CreatorType>
    CreatorType* GetExistingNodeCreator()
    {
        if (!_nodeStack.empty()) {
            if (CreatorType* existingCreator = 
                    dynamic_cast<CreatorType*>(_nodeStack.back().get())) {
                return existingCreator;
            }
        }
        return nullptr;
    }

    // Return pointer to the node creator on the top of the stack if
    // it is an instance of CreatorType, otherwise construct a new
    // CreatorType, push it onto the stack, and return it.
    template <class CreatorType>
    CreatorType* GetNodeCreator() 
    {
        CreatorType* creator = GetExistingNodeCreator<CreatorType>();
        if (!creator) {
            creator = PushNodeCreator<CreatorType>();
        }
        return creator;
    }

    // Pop the node creator from the top of the stack and use it to
    // create an expression node.
    std::unique_ptr<Impl::Node> CreateExpressionNode(std::string* errMsg)
    {
        if (!TF_VERIFY(!_nodeStack.empty()) || !TF_VERIFY(_nodeStack.back())) {
            *errMsg = "Unknown error";
            return nullptr;
        }

        std::unique_ptr<NodeCreator> creator = std::move(_nodeStack.back());
        _nodeStack.pop_back();

        return creator->CreateNode(errMsg);
    }

    // Pop the node creator from the top of the stack and use it to
    // create an AST node.
    std::unique_ptr<ASTNodes::Node>
    CreateASTNode(std::string* errMsg)
    {
        if (!TF_VERIFY(!_nodeStack.empty()) || !TF_VERIFY(_nodeStack.back())) {
            *errMsg = "Unknown error";
            return nullptr;
        }

        std::unique_ptr<NodeCreator> creator = std::move(_nodeStack.back());
        _nodeStack.pop_back();

        return creator->CreateASTNode(errMsg);
    }

private:
    std::vector<std::unique_ptr<NodeCreator>> _nodeStack;
};

// Parser utilities -----------------------------------------------

template <typename Input>
[[noreturn]]
void _ThrowParseError(const Input& in, const std::string& msg)
{
    // XXX: 
    // As of pegtl 2.x, the commented out code below prepends the position
    // into the exception's error string with no way to recover just the error
    // itself. The c'tor that takes a std::vector<position> avoids this,
    // allowing us to format the position ourselves in our exception handler.
    //
    // pegtl 3.x adds API to parse_error to decompose the error message,
    // so we could use that later.

    throw parse_error(msg, in); 
}

// Parser grammar -----------------------------------------------
// Parsing rules for the expression grammar.

// XXX: 
// When given a variable with illegal characters, like "${FO-OO}",
// this rule yields a confusing error message stating that there's a
// missing "}". This is because it recognizes everything up to the
// illegal character as the variable and expects to find the
// closing "}" after it. It'd be nice to fix this.
struct VariableStart 
    : string<'$', '{'>
{};

struct VariableEnd
    : string<'}'>
{};

template <class C>
struct VariableName
    : identifier
{};

template <class C>
struct VariableImpl
    : if_must<
        VariableStart,
        VariableName<C>,
        VariableEnd
    > 
{
    using Name = VariableName<C>;
};

// Variable reference at the top level of an expression.
struct Variable
    : VariableImpl<Variable>
{};

// ----------------------------------------

// Variable reference in a quoted string.
struct QuotedStringVariable
    : VariableImpl<QuotedStringVariable>
{};

// Characters that must be escaped in a quoted string.
template <char QuoteChar>
struct QuotedStringEscapedChar
    : one<'`', '$', '\\', QuoteChar>
{};

// Sequence of allowed characters in a quoted string.
template <char QuoteChar>
struct QuotedStringChars
    : plus<
        sor<
            // An escaped character.
            seq<
                one<'\\'>, QuotedStringEscapedChar<QuoteChar>
            >,
            // Any other characters that aren't the start of a stage
            // variable or the quote character, since those are handled
            // by different rules.
            seq<
                not_at<sor<VariableStart, one<QuoteChar>>>, 
                any
            >
        >
    >
{};

template <char QuoteChar>
struct QuotedStringStart
    : string<QuoteChar>
{};

template <char QuoteChar>
struct QuotedStringEnd
    : string<QuoteChar>
{};

template <char QuoteChar>
struct QuotedStringBody
    : star<
        sor<
            QuotedStringVariable,
            QuotedStringChars<QuoteChar>
        >
    >
{};

template <char QuoteChar>
struct QuotedString
    : if_must<
        QuotedStringStart<QuoteChar>, 
        QuotedStringBody<QuoteChar>,
        QuotedStringEnd<QuoteChar>
    >
{
    using Start = QuotedStringStart<QuoteChar>;
    using Body = QuotedStringBody<QuoteChar>;
    using End = QuotedStringEnd<QuoteChar>;
};

using DoubleQuotedString = QuotedString<'"'>;
using SingleQuotedString = QuotedString<'\''>;

// ----------------------------------------

struct Integer
    : seq<
        opt<one<'-'>>,
        plus<ascii::digit>
    >
{};

// ----------------------------------------

// We allow "True", "true", "False", "false" because these
// are the representations used in the two primary languages supported
// by USD -- C++ and Python -- and that correspondence may make it easier
// for users working in those languages while writing expressions.
struct BooleanTrue
    : sor<
        PXR_PEGTL_KEYWORD("True"),
        PXR_PEGTL_KEYWORD("true")
    >
{};

struct BooleanFalse
    : sor<
        PXR_PEGTL_KEYWORD("False"),
        PXR_PEGTL_KEYWORD("false")
    >
{};

struct Boolean
    : sor<
        BooleanTrue,
        BooleanFalse
    >
{};

// ----------------------------------------

struct None
    : sor<
        PXR_PEGTL_KEYWORD("None"),
        PXR_PEGTL_KEYWORD("none")
    >
{};

// ----------------------------------------

// Forward-declare to allow expression rule to be used for function arguments.
struct ExpressionBody;

struct FunctionName
    : identifier
{};

struct FunctionArgumentStart
    : pad<one<'('>, one<' '>>
{};

struct FunctionArgumentEnd
    : pad<one<')'>, one<' '>>
{};

// A function argument can be any valid expression. We can't directly
// derive from ExpressionBody because doing so would require ExpressionBody
// to be a complete type. We use a templated wrapper class to work around
// this requirement instead.
template <class Base>
struct FunctionArgumentWrapper
    : public Base
{};

using FunctionArgument = FunctionArgumentWrapper<ExpressionBody>;

// Function arguments are zero or more comma-separated arguments.
struct FunctionArguments
    : sor<
        list<FunctionArgument, one<','>, one<' '>>,
        star<one<' '>>
    >
{};

struct Function
    : if_must<
        seq<FunctionName, FunctionArgumentStart>,
        FunctionArguments,
        FunctionArgumentEnd
    >
{};

// ----------------------------------------

struct ScalarExpression
    : sor<
        Variable,
        DoubleQuotedString,
        SingleQuotedString,
        Integer,
        Boolean,
        None,
        Function
    >
{};

// ----------------------------------------

struct ListStart
    : one<'['>
{};

struct ListEnd
    : one<']'>
{};

struct ListElement
    : public ScalarExpression
{};

struct ListElements
    : sor<
        list<ListElement, one<','>, one<' '>>,
        star<one<' '>>
    >
{};

struct ListExpression
    : if_must<
        ListStart, 
        ListElements,
        ListEnd>
{};

// ----------------------------------------

struct ExpressionStart
    : string<'`'>
{};

struct ExpressionEnd
    : string<'`'>
{};

struct ExpressionBody
    : sor<
        ScalarExpression,
        ListExpression
    >
{};

struct Expression
    : must<
        ExpressionStart,
        ExpressionBody,
        ExpressionEnd
    > 
{};

// Parser actions ---------------------------------------------
// Objects that define the actions to take when a parsing rule
// is matched.

template <typename Rule>
struct Action
{};

template <char QuoteChar>
struct Action<QuotedStringChars<QuoteChar>>
{
    template <typename ActionInput>
    static void apply(const ActionInput& in, ParserContext& context)
    {
        context.GetNodeCreator<StringNodeCreator>()
            ->parts.push_back({ in.string(), /* isVar = */ false });
    }
};

template <>
struct Action<QuotedStringVariable::Name>
{
    template <typename ActionInput>
    static void apply(const ActionInput& in, ParserContext& context)
    {
        context.GetNodeCreator<StringNodeCreator>()
            ->parts.push_back({ in.string(), /* isVar = */ true });
    }
};

template <char QuoteChar>
struct Action<QuotedStringStart<QuoteChar>>
{
    template <typename ActionInput>
    static void apply(const ActionInput& in, ParserContext& context)
    {
        // We need to make sure that a StringNodeCreator exists by the time
        // we finish parsing the quoted string. This typically happens in
        // the QuotedStringAction subclasses, but if the string being parsed
        // is empty we'll never activate those actions. So we create the
        // StringNodeCreator here but leave it empty.
        context.GetNodeCreator<StringNodeCreator>();
    }
};

template <>
struct Action<Variable::Name>
{
    template <typename ActionInput>
    static void apply(const ActionInput& in, ParserContext& context)
    {
        context.GetNodeCreator<VariableNodeCreator>()->var = in.string();
    }
};

template <>
struct Action<Integer>
{
    template <typename ActionInput>
    static void apply(const ActionInput& in, ParserContext& context)
    {
        bool outOfRange = false;
        const int64_t value = TfStringToInt64(in.string(), &outOfRange);
        if (outOfRange) {
            _ThrowParseError(
                in, TfStringPrintf("Integer %s out of range.", 
                    in.string().c_str()));
        }

        context.GetNodeCreator<IntegerNodeCreator>()->value = value;
    }
};

template <>
struct Action<BooleanTrue>
{
    template <typename ActionInput>
    static void apply(const ActionInput& in, ParserContext& context)
    {
        context.GetNodeCreator<BoolNodeCreator>()->value = true;
    }
};

template <>
struct Action<BooleanFalse>
{
    template <typename ActionInput>
    static void apply(const ActionInput& in, ParserContext& context)
    {
        context.GetNodeCreator<BoolNodeCreator>()->value = false;
    }
};

template <>
struct Action<None>
{
    template <typename ActionInput>
    static void apply(const ActionInput& in, ParserContext& context)
    {
        context.GetNodeCreator<NoneNodeCreator>();
    }
};

template<>
struct Action<ListStart>
{
    template <typename ActionInput>
    static void apply(const ActionInput& in, ParserContext& context)
    {
        context.PushNodeCreator<ListNodeCreator>();
    }
};

template<>
struct Action<ListElement>
{
    template <typename ActionInput>
    static void apply(const ActionInput& in, ParserContext& context)
    {
        std::unique_ptr<NodeCreator> elemCreator = context.PopNodeCreator();
        if (!elemCreator) {
            _ThrowParseError(in, 
                "Internal error: could not pop node creator stack");
        }

        ListNodeCreator* listCreator =
            context.GetExistingNodeCreator<ListNodeCreator>();
        if (!listCreator) {
            _ThrowParseError(in,
                "Internal error: list creator not at top of stack");
        }

        listCreator->elements.push_back(std::move(elemCreator));
    }
};

template<>
struct Action<FunctionName>
{
    template <typename ActionInput>
    static void apply(const ActionInput& in, ParserContext& context)
    {
        context.PushNodeCreator<FunctionNodeCreator>(in.string());
    }
};

template<>
struct Action<FunctionArgument>
{
    template <typename ActionInput>
    static void apply(const ActionInput& in, ParserContext& context)
    {
        std::unique_ptr<NodeCreator> elemCreator = context.PopNodeCreator();
        if (!elemCreator) {
            _ThrowParseError(in,
                "Internal error: could not pop node creator stack");
        }

        FunctionNodeCreator* fnCreator =
            context.GetExistingNodeCreator<FunctionNodeCreator>();
        if (!fnCreator) {
            _ThrowParseError(in,
                "Internal error: function creator not at top of stack");
        }

        fnCreator->functionArgs.push_back(std::move(elemCreator));
    }
};

// Parser error messages ----------------------------------------

template <typename Rule>
struct Errors
    : public normal<Rule>
{
    static const std::string errorMsg;

    template <typename Input, typename... States>
    [[noreturn]] static void raise(const Input& in, States&&...)
    {
        _ThrowParseError(in, errorMsg);
    }
};

#define MATCH_ERROR(rule, msg)                                  \
    template <> const std::string Errors<rule>::errorMsg = msg;

// Should never hit these errors because of how the rules are defined.
MATCH_ERROR(ListElements, "");
MATCH_ERROR(FunctionArguments, "");

MATCH_ERROR(ListEnd, "Missing ending ']'");
MATCH_ERROR(FunctionArgumentEnd, "Missing ending ')'");
MATCH_ERROR(ExpressionStart, "Expressions must begin with '`'");
MATCH_ERROR(ExpressionBody, "Unexpected expression");
MATCH_ERROR(ExpressionEnd, "Missing ending '`'");

MATCH_ERROR(
    Variable::Name, "Variables must be a C identifier");
MATCH_ERROR(
    QuotedStringVariable::Name, "Variables must be a C identifier");
MATCH_ERROR(VariableEnd, "Missing ending '}'");

MATCH_ERROR(DoubleQuotedString::Body, "Invalid string contents");
MATCH_ERROR(DoubleQuotedString::End, R"(Missing ending '"')");

MATCH_ERROR(SingleQuotedString::Body, "Invalid string contents");
MATCH_ERROR(SingleQuotedString::End, R"(Missing ending "'")");

struct ParseResult
{
    std::optional<ParserContext> context;
    std::vector<std::string> errMsg;
};

} // end anonymous namespace

static ParseResult
Sdf_ParseVariableExpressionImpl(const std::string& expr)
{
    namespace pegtl = PXR_PEGTL_NAMESPACE;

    ParserContext context;

    pegtl::memory_input<> in(expr, "");

    try {
        const bool parseSuccess =
            TfDebug::IsEnabled(SDF_VARIABLE_EXPRESSION_PARSING) ?
            pegtl::standard_trace<Expression, Action, Errors>(in, context) :
            pegtl::parse<Expression, Action, Errors>(in, context);
        
        if (!parseSuccess) {
            return { std::nullopt, { "Unable to parse expression" } };
        }
    }
    catch (const pegtl::parse_error& e) {
        return { 
            std::nullopt,

            // XXX: "at character" is probably incorrect if the expression
            // contains Unicode strings?
            { TfStringPrintf("%s at character %zu",
                             std::string(e.message()).c_str(),
                             e.positions().empty()
                             ? expr.size() : e.positions().front().column-1) }
        };
     }

    return { std::move(context), {} };
}

Sdf_VariableExpressionParserResult
Sdf_ParseVariableExpression(const std::string& expr)
{
    ParseResult result = Sdf_ParseVariableExpressionImpl(expr);
    if (!result.context) {
        return { nullptr, std::move(result.errMsg) };
    }

    std::string errMsg;
    std::unique_ptr<Impl::Node> exprNode = 
        result.context->CreateExpressionNode(&errMsg);

    if (!exprNode) {
        return { nullptr, { std::move(errMsg) } };
    }

    return { std::move(exprNode), {} };
}

Sdf_VariableExpressionASTParserResult
Sdf_ParseVariableExpressionAST(const std::string& expr)
{
    ParseResult result = Sdf_ParseVariableExpressionImpl(expr);
    if (!result.context) {
        return { nullptr, std::move(result.errMsg) };
    }

    std::string errMsg;
    std::unique_ptr<ASTNodes::Node> astNode = 
        result.context->CreateASTNode(&errMsg);

    if (!astNode) {
        return { nullptr, { std::move(errMsg) } };
    }

    return { std::move(astNode), {} };
}

bool
Sdf_IsVariableExpression(const std::string& s)
{
    return s.length() > 2 && s.front() == '`' && s.back() == '`';
}

PXR_NAMESPACE_CLOSE_SCOPE
