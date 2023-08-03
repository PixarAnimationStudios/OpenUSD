//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/variableExpressionParser.h"

#include "pxr/usd/sdf/debugCodes.h"
#include "pxr/usd/sdf/variableExpressionImpl.h"

#include "pxr/base/tf/pxrPEGTL/pegtl.h"
#include "pxr/base/tf/stringUtils.h"

using namespace tao::TAO_PEGTL_NAMESPACE;

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

namespace Impl = Sdf_VariableExpressionImpl;

// Node creators -----------------------------------------------
// These objects are responsible for saving intermediate values as
// an expression is being parsed and using them to create expression
// nodes.

class NodeCreator
{
public:
    virtual ~NodeCreator();
    virtual std::unique_ptr<Impl::Node> CreateNode() = 0;
};

NodeCreator::~NodeCreator() = default;

class StringNodeCreator
    : public NodeCreator
{
public:
    std::unique_ptr<Impl::Node> CreateNode() override
    {
        return std::make_unique<Impl::StringNode>(std::move(parts));
    }
    
    using Part = Impl::StringNode::Part;
    std::vector<Part> parts;
};

class VariableNodeCreator
    : public NodeCreator
{
public:
    std::unique_ptr<Impl::Node> CreateNode() override
    {
        return std::make_unique<Impl::VariableNode>(std::move(var));
    }
    
    std::string var;
};

template <class Type>
class ConstantNodeCreator
    : public NodeCreator
{
public:
    std::unique_ptr<Impl::Node> CreateNode() override
    {
        return std::make_unique<Impl::ConstantNode<Type>>(value);
    }
    
    Type value;
};

using IntegerNodeCreator = ConstantNodeCreator<int64_t>;
using BoolNodeCreator = ConstantNodeCreator<bool>;

class NoneNodeCreator
    : public NodeCreator
{
public:
    std::unique_ptr<Impl::Node> CreateNode() override
    {
        return std::make_unique<Impl::NoneNode>();
    }
};

class ListNodeCreator
    : public NodeCreator
{
public:
    std::unique_ptr<Impl::Node> CreateNode() final
    {
        return std::make_unique<Impl::ListNode>(std::move(elements));
    };

    std::vector<std::unique_ptr<Impl::Node>> elements;
};

// Parser state -----------------------------------------------
// Objects responsible for keeping track of intermediate state as an
// expression is being parsed.

class ParserContext
{
public:
    // Create and push a node creator of type CreatorType onto the stack,
    // passing in any given args to CreatorType c'tor, and return it.
    template <class CreatorType, class... Args>
    CreatorType* PushNodeCreator(const Args&... args)
    {
        _nodeStack.push_back(std::make_unique<CreatorType>(args...));
        return static_cast<CreatorType*>(_nodeStack.back().get());
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

        return creator->CreateNode();
    }

private:
    std::vector<std::unique_ptr<NodeCreator>> _nodeStack;
};

// Parser utilities -----------------------------------------------

template <typename Input>
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

    // throw parse_error(errorMsg, in); 
    throw parse_error(msg, std::vector<position>{ in.position() });
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
        TAO_PEGTL_KEYWORD("True"),
        TAO_PEGTL_KEYWORD("true")
    >
{};

struct BooleanFalse
    : sor<
        TAO_PEGTL_KEYWORD("False"),
        TAO_PEGTL_KEYWORD("false")
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
        TAO_PEGTL_KEYWORD("None"),
        TAO_PEGTL_KEYWORD("none")
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
        None
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
        std::string errMsg;
        std::unique_ptr<Impl::Node> elemNode = 
            context.CreateExpressionNode(&errMsg);
        if (!elemNode) {
            _ThrowParseError(in, errMsg);
        }

        ListNodeCreator* listCreator =
            context.GetExistingNodeCreator<ListNodeCreator>();
        if (!listCreator) {
            _ThrowParseError(in, "Unknown error");
        }

        listCreator->elements.push_back(std::move(elemNode));
    }
};

// Parser error messages ----------------------------------------

template <typename Rule>
struct Errors
    : public normal<Rule>
{
    static const std::string errorMsg;

    template <typename Input, typename... States>
    static void raise(const Input& in, States&&...)
    {
        _ThrowParseError(in, errorMsg);
    }
};

#define MATCH_ERROR(rule, msg)                                  \
    template <> const std::string Errors<rule>::errorMsg = msg;

// Should never hit this error because of how the rule is defined.
MATCH_ERROR(ListElements, "");

MATCH_ERROR(ListEnd, "Missing ending ']'");
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

} // end anonymous namespace

Sdf_VariableExpressionParserResult
Sdf_ParseVariableExpression(const std::string& expr)
{
    namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

    ParserContext context;

    pegtl::memory_input<> in(expr, "");

    try {
        const bool parseSuccess =
            TfDebug::IsEnabled(SDF_VARIABLE_EXPRESSION_PARSING) ?
            pegtl::parse<Expression, Action, 
                         pegtl::trace<Errors>::control>(in, context) :
            pegtl::parse<Expression, Action, Errors>(in, context);
        
        if (!parseSuccess) {
            return { nullptr, { "Unable to parse expression" } };
        }
    }
    catch (const pegtl::parse_error& e) {
        return { 
            nullptr, 

            // XXX: "at character" is probably incorrect if the expression
            // contains Unicode strings?
            { TfStringPrintf("%s at character %zu", 
                e.what(), 
                e.positions.empty() ? 
                    expr.size() : e.positions.front().byte_in_line) }
        };
     }

    std::string errMsg;
    std::unique_ptr<Impl::Node> exprNode = 
        context.CreateExpressionNode(&errMsg);

    if (!exprNode) {
        return { nullptr, { std::move(errMsg) } };
    }

    return { std::move(exprNode), {} };
}

bool
Sdf_IsVariableExpression(const std::string& s)
{
    return s.length() > 2 && s.front() == '`' && s.back() == '`';
}

PXR_NAMESPACE_CLOSE_SCOPE
