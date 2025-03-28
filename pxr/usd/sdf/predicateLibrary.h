//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_PREDICATE_LIBRARY_H
#define PXR_USD_SDF_PREDICATE_LIBRARY_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/functionTraits.h"
#include "pxr/base/tf/pxrTslRobinMap/robin_map.h"
#include "pxr/base/vt/value.h"

#include "pxr/usd/sdf/predicateExpression.h"

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdfPredicateParamNamesAndDefaults
///
/// Represents named function parameters, with optional default values.  These
/// are generally constructed via an initializer_list and specified in
/// SdfPredicateLibrary::Define().
///
/// Valid parameter names and defaults have non-empty names, and all parameters
/// following the first one with a default value must also have default values.
struct SdfPredicateParamNamesAndDefaults {
    
    /// \class Param represents a single named parameter with an optional
    /// default value.
    struct Param {
        /// Construct with or implicitly convert from name.
        Param(char const *name) : name(name) {}
        
        /// Construct from name and default value.
        template <class Val>
        Param(char const *name, Val &&defVal)
            : name(name), val(std::forward<Val>(defVal)) {}

        std::string name;
        VtValue val;
    };

    /// Default constructor produces empty set of names & defaults.
    SdfPredicateParamNamesAndDefaults() : _numDefaults(0) {}

    /// Construct or implicitly convert from initializer_list<Param>.
    SdfPredicateParamNamesAndDefaults(
        std::initializer_list<Param> const &params)
        : _params(params.begin(), params.end())
        , _numDefaults(_CountDefaults()) {}

    /// Check that all parameters have non-empty names and that all paramters
    /// following the first with a default value also have default values.
    /// Issue TF_CODING_ERROR()s and return false if these conditions are
    /// violated, otherwise return true.
    SDF_API
    bool CheckValidity() const;

    /// Return a reference to the parameters in a vector.
    std::vector<Param> const &GetParams() const & {
        return _params;
    }

    /// Move-return the parameters in a vector.
    std::vector<Param> GetParams() const && {
        return std::move(_params);
    }

    /// Return the number of params with default values.
    size_t GetNumDefaults() const {
        return _numDefaults;
    }

private:
    SDF_API
    size_t _CountDefaults() const;
    
    std::vector<Param> _params;
    size_t _numDefaults;
};


/// \class SdfPredicateFunctionResult
///
/// Represents the result of a predicate function: a pair of the boolean result
/// and a Constancy token indicating whether the function result is constant
/// over "descendant" objects, or that it might vary over "descendant" objects.
class SdfPredicateFunctionResult
{
public:
    enum Constancy { ConstantOverDescendants, MayVaryOverDescendants };

    /// Default construction produces a 'false' result that
    /// 'MayVaryOverDescendants'.
    constexpr SdfPredicateFunctionResult()
        : _value(false), _constancy(MayVaryOverDescendants) {}

    /// Construct with \p value and \p MayVaryOverDescendants constancy.
    explicit SdfPredicateFunctionResult(bool value)
        : SdfPredicateFunctionResult(value, MayVaryOverDescendants) {}

    /// Construct with \p value and \p constancy.
    SdfPredicateFunctionResult(bool value, Constancy constancy)
        : _value(value), _constancy(constancy) {}

    /// Create with \p value and 'ConstantOverDescendants'
    static SdfPredicateFunctionResult MakeConstant(bool value) {
        return { value, ConstantOverDescendants };
    }
    
    /// Create with \p value and 'MayVaryOverDescendants'
    static SdfPredicateFunctionResult MakeVarying(bool value) {
        return { value, MayVaryOverDescendants };
    }

    /// Return the result value.
    bool GetValue() const {
        return _value;
    }

    /// Return the result constancy.
    Constancy GetConstancy() const {
        return _constancy;
    }

    /// Return true if this result's constancy is ConstantOverDescendants.
    bool IsConstant() const {
        return GetConstancy() == ConstantOverDescendants;
    }

#if !defined(doxygen)
    using UnspecifiedBoolType = bool (SdfPredicateFunctionResult::*);
#endif //!doxygen

    /// Return GetValue().
    operator UnspecifiedBoolType() const {
        return _value ? &SdfPredicateFunctionResult::_value : nullptr;
    }

    /// Return a result with the opposite value but the same constancy.
    SdfPredicateFunctionResult operator!() const {
        return { !_value, _constancy };
    }

    /// Set this result's value to \p other's value, and propagate constancy; if
    /// both this and \p other are ConstantOverDescendants, this object's
    /// constancy remains ConstantOverDescendants.  Otherwise set this object's
    /// constancy to MayVaryOverDescendants.
    void SetAndPropagateConstancy(SdfPredicateFunctionResult other) {
        _value = other._value;
        if (_constancy == ConstantOverDescendants &&
            other._constancy == MayVaryOverDescendants) {
            _constancy = MayVaryOverDescendants;
        }
    }

private:
    friend bool operator==(SdfPredicateFunctionResult lhs,
                           SdfPredicateFunctionResult rhs) {
        return lhs._value == rhs._value &&
            lhs._constancy == rhs._constancy;
    }
    friend bool operator!=(SdfPredicateFunctionResult lhs,
                           SdfPredicateFunctionResult rhs) {
        return !(lhs == rhs);
    }
    
    friend bool operator==(SdfPredicateFunctionResult pfr, bool rhs) {
        return pfr._value == rhs;
    }
    friend bool operator==(bool lhs, SdfPredicateFunctionResult pfr) {
        return lhs == pfr._value;
    }
    friend bool operator!=(SdfPredicateFunctionResult pfr, bool rhs) {
        return pfr._value != rhs;
    }
    friend bool operator!=(bool lhs, SdfPredicateFunctionResult pfr) {
        return lhs != pfr._value;
    }
    
    bool _value;
    Constancy _constancy;
};

// fwd decl
template <class DomainType>
class SdfPredicateProgram;

// fwd decl
template <class DomainType>
class SdfPredicateLibrary;

// fwd decl
template <class DomainType>
SdfPredicateProgram<DomainType>
SdfLinkPredicateExpression(SdfPredicateExpression const &expr,
                           SdfPredicateLibrary<DomainType> const &lib);

/// \class SdfPredicateLibrary
///
/// Represents a library of predicate functions for use with
/// SdfPredicateExpression.  Call SdfLinkPredicateExpression() with an
/// expression and a library to produce a callable SdfPredicateProgram.
template <class DomainType>
class SdfPredicateLibrary
{
    friend SdfPredicateProgram<DomainType>
    SdfLinkPredicateExpression<DomainType>(
        SdfPredicateExpression const &expr,
        SdfPredicateLibrary const &lib);

    using NamesAndDefaults = SdfPredicateParamNamesAndDefaults;

public:
    /// The type of a bound function, the result of binding passed arguments.
    using PredicateFunction =
        std::function<SdfPredicateFunctionResult (DomainType const &)>;

    /// Default constructor produces an empty library.
    SdfPredicateLibrary() = default;

    /// Move-construct from an \p other library.
    SdfPredicateLibrary(SdfPredicateLibrary &&other) = default;
    
    /// Copy-construct from an \p other library.
    SdfPredicateLibrary(SdfPredicateLibrary const &other) {
        for (auto iter = other._binders.begin(), end = other._binders.end();
             iter != end; ++iter) {
            auto &theseBinders = _binders[iter->first];
            for (auto const &otherBinder: iter->second) {
                theseBinders.push_back(otherBinder->Clone());
            }
        }
    }

    /// Move-assignment from an \p other library.
    SdfPredicateLibrary &operator=(SdfPredicateLibrary &&other) = default;

    /// Copy-assignment from an \p other library.
    SdfPredicateLibrary &operator=(SdfPredicateLibrary const &other) {
        if (this != &other) {
            SdfPredicateLibrary copy(other);
            *this = std::move(copy);
        }
        return *this;
    }
    
    /// Register a function with name \p name in this library.  The first
    /// argument must accept a DomainType instance.  The remaining arguments
    /// must be convertible from bool, int, float, string.
    template <class Fn>
    SdfPredicateLibrary &Define(char const *name, Fn &&fn) {
        return Define(name, std::forward<Fn>(fn), {});
    }
         
    /// Register a function with name \p name in this library.  The first
    /// argument must accept a DomainType instance.  The remaining arguments
    /// must be convertible from bool, int, float, string.  Optional parameter
    /// names and default values may be supplied in \p namesAndDefaults.
    template <class Fn>
    SdfPredicateLibrary &
    Define(std::string const &name, Fn &&fn,
           NamesAndDefaults const &namesAndDefaults) {
        // Try to create a new overload binder for 'name'.  The main operation a
        // binder does is, when "linking" a predicate expression, given a
        // specific set of arguments from the expression, check to see if those
        // arguments can be bound to 'fn', and if so return a type-erased
        // callable that invokes fn with those arguments.
        if (auto obinder = _OverloadBinder<std::decay_t<Fn>>
            ::TryCreate(std::forward<Fn>(fn), namesAndDefaults)) {
            _binders[name].push_back(std::move(obinder));
        }
        return *this;
    }

    /// Register a custom binding function for \p name in this library.  The
    /// function must take a single argument of type
    /// std::vector<SdfPredicateExpression::FnArg>.  When invoked, it must
    /// attempt to bind the arguments passed in the vector and return a bound
    /// PredicateFunction object.  If the arguments are invalid, return an empty
    /// PredicateFunction.
    template <class Fn>
    SdfPredicateLibrary &
    DefineBinder(std::string const &name, Fn &&fn) {
        auto binder = _CustomBinder<
            std::decay_t<Fn>>::Create(std::forward<Fn>(fn));
        _binders[name].push_back(std::move(binder));
        return *this;
    }

private:

    PredicateFunction
    _BindCall(std::string const &name,
              std::vector<SdfPredicateExpression::FnArg> const &args) const {
        PredicateFunction ret;
        auto iter = _binders.find(name);
        if (iter == _binders.end()) {
            TF_RUNTIME_ERROR("No registered function '%s'", name.c_str());
            return ret;
        }
        // Run thru optimistically first -- if we fail to bind to any overload,
        // then produce an error message with all the overload signatures.
        for (auto i = iter->second.rbegin(),
                 end = iter->second.rend(); i != end; ++i) {
            ret = (*i)->Bind(args);
            if (ret) {
                break;
            }
        }
        return ret;
    }
    
    template <class ParamType>
    static void _CheckOneNameAndDefault(
        bool &valid, size_t index, size_t numParams,
        NamesAndDefaults const &namesAndDefaults) {

        // If the namesIndex-th param has a default, it must be convertible to
        // the ArgIndex-th type.
        std::vector<NamesAndDefaults::Param> const &
            params = namesAndDefaults.GetParams();
        
        size_t nFromEnd = numParams - index - 1;
        if (nFromEnd >= params.size()) {
            // No more names & defaults to check.
            return;
        }

        size_t namesIndex = params.size() - nFromEnd - 1;
        
        auto const &param = params[namesIndex];
        if (!param.val.IsEmpty() && !param.val.CanCast<ParamType>()) {
            TF_CODING_ERROR("Predicate default parameter '%s' value of "
                            "type '%s' cannot convert to c++ argument of "
                            "type '%s' at index %zu",
                            param.name.c_str(),
                            param.val.GetTypeName().c_str(),
                            ArchGetDemangled<ParamType>().c_str(),
                            index);
            valid = false;
        }
    }
    
    template <class ParamsTuple, size_t... I>
    static bool
    _CheckNamesAndDefaultsImpl(
        NamesAndDefaults const &namesAndDefaults,
        std::index_sequence<I...>) {
        // A fold expression would let us just do &&, but that's c++'17, so we
        // just do all of them and set a bool.
        bool valid = true;
        constexpr size_t N = std::tuple_size<ParamsTuple>::value;
        // Need an unused array so we can use an initializer list to invoke
        // _CheckOneNameAndDefault N times.
        int unused[] = {
            0,
            (_CheckOneNameAndDefault<std::tuple_element_t<N-I-1, ParamsTuple>>(
                valid, N-I-1, N, namesAndDefaults), 0)...
        };
        TF_UNUSED(unused);
        return valid;
    }
    
    template <class Fn>
    static bool
    _CheckNamesAndDefaultsWithSignature(
        NamesAndDefaults const &namesAndDefaults) {
        // Basic check for declared names & defaults.
        if (!namesAndDefaults.CheckValidity()) {
            return false;
        }

        using Traits = TfFunctionTraits<Fn>;

        // Return type must convert to bool.
        static_assert(
            std::is_same<typename Traits::ReturnType,
                         SdfPredicateFunctionResult>::value ||
            std::is_convertible<
                typename Traits::ReturnType, bool>::value, "");

        // Fn must have at least one argument, and DomainType must be
        // convertible to the first arg.
        using DomainArgType = typename Traits::template NthArg<0>;
        static_assert(
            std::is_convertible<DomainType, DomainArgType>::value, "");

        // Issue an error if there are more named arguments than c++ function
        // arguments.  Subtract one from Arity to account for the leading
        // DomainType argument.
        std::vector<NamesAndDefaults::Param> const &
            params = namesAndDefaults.GetParams();
        if (params.size() > Traits::Arity-1) {
            TF_CODING_ERROR("Predicate named arguments (%zu) exceed number of "
                            "C++ function arguments (%zu)",
                            params.size(), Traits::Arity-1);
            return false;
        }
        
        // Now check the names and defaults against the Fn signature, from back
        // to front, since namesAndDefaults must be "right-aligned" -- that is,
        // any unnamed arguments must come first.
        if (!params.empty()) {
            // Strip DomainType arg...
            using FullParams = typename Traits::ArgTypes;
            using Params =
                TfMetaApply<TfMetaDecay, TfMetaApply<TfMetaTail, FullParams>>;
            using ParamsTuple = TfMetaApply<std::tuple, Params>;

            return _CheckNamesAndDefaultsImpl<ParamsTuple>(
                namesAndDefaults, std::make_index_sequence<Traits::Arity-1> {});
        }
        return true;
    }

    template <class ParamType>
    static void _TryBindOne(
        size_t index, size_t numParams,
        ParamType &param,
        bool &boundAllParams,
        std::vector<SdfPredicateExpression::FnArg> const &args,
        std::vector<bool> &boundArgs,
        NamesAndDefaults const &namesAndDefaults) {

        // Bind the index-th 'param' from 'args' &
        // 'namesAndDefaults'. 'boundArgs' corresponds to 'args' and indicates
        // which have already been bound.  This function sets one bit in
        // 'boundArgs' if it binds one of them to a parameter.  It may bind a
        // default from 'namesAndDefaults', in which case it sets no bit.  If no
        // suitable binding can be determined for this parameter, set
        // 'boundAllParams' false.

        // If we've already failed to bind, just return early.
        if (!boundAllParams) {
            return;
        }

        // namesAndDefaults covers trailing parameters -- that is, there may be
        // zero or more leading unnamed parameters.
        std::vector<NamesAndDefaults::Param> const &
            params = namesAndDefaults.GetParams();
        size_t numUnnamed = params.size() - numParams;
        NamesAndDefaults::Param const *paramNameAndDefault = nullptr;
        if (index >= numUnnamed) {
            paramNameAndDefault = &params[index - numUnnamed];
        }

        // If this is a purely positional parameter (paramNameAndDefault is
        // nullptr) or the caller supplied a positional arg (unnamed) then we
        // use index-correspondence.
        auto const *posArg =
            (index < args.size() && args[index].argName.empty()) ?
            &args[index] : nullptr;

        auto tryBind = [&](VtValue const &val, size_t argIndex) {
            VtValue cast = VtValue::Cast<ParamType>(val);
            if (!cast.IsEmpty()) {
                param = cast.UncheckedRemove<ParamType>();
                boundArgs[argIndex] = true;
                return true;
            }
            boundAllParams = false;
            return false;
        };
        
        if (!paramNameAndDefault) {
            // If this is a positional parameter, the arg must be too.
            if (!posArg || !posArg->argName.empty()) {
                boundAllParams = false;
                return;
            }
            // Try to bind posArg.
            tryBind(posArg->value, index);
            return;
        }
        else if (posArg) {
            // Passed a positional arg, try to bind.
            tryBind(posArg->value, index);
            return;
        }

        // Only possibility is a keyword arg.  If there's a matching name, try
        // to bind that, otherwise try to fill a default.
        for (size_t i = 0, end = args.size(); i != end; ++i) {
            if (boundArgs[i]) {
                // Already bound.
                continue;
            }
            if (args[i].argName == paramNameAndDefault->name) {
                // Matching name -- try to bind.
                tryBind(args[i].value, i);
                return;
            }
        }

        // No matching arg, try to fill default val.
        VtValue cast = VtValue::Cast<ParamType>(paramNameAndDefault->val);
        if (!cast.IsEmpty()) {
            param = cast.UncheckedRemove<ParamType>();
        }
        else {
            // Error, could not fill default.
            boundAllParams = false;
        }
    }
    
    template <class ParamsTuple, size_t... I>
    static bool
    _TryBindArgs(ParamsTuple &params,
                 std::vector<SdfPredicateExpression::FnArg> const &args,
                 NamesAndDefaults const &namesAndDefaults,
                 std::index_sequence<I...>,
                 std::vector<bool> &boundArgs) {

        // A fold expression would let us just do &&, but that's '17, so we just
        // do all of them and set a bool.
        bool bound = true;
        boundArgs.assign(args.size(), false);
        // Need a unused array so we can use an initializer list to invoke
        // _TryBindOne N times.
        int unused[] = {
            0,
            (_TryBindOne(I, std::tuple_size<ParamsTuple>::value,
                         std::get<I>(params), bound,
                         args, boundArgs, namesAndDefaults), 0)...
        };
        TF_UNUSED(unused);
        return bound;
    }

    template <class Tuple>
    static void
    _FillArbitraryArgs(std::true_type,
                       std::vector<SdfPredicateExpression::FnArg> const &args,
                       std::vector<bool> const &boundArgs,
                       Tuple &typedArgs) {
        std::vector<SdfPredicateExpression::FnArg> &rest =
            std::get<std::tuple_size<Tuple>::value-1>(typedArgs);
        // 'boundArgs' and 'args' correspond.  Fill 'rest' with the elements of
        // 'args' for which the corresponding element of 'boundArgs' is false,
        // in order.
        rest.clear();
        for (size_t i = 0; i != args.size(); ++i) {
            if (!boundArgs[i]) {
                rest.push_back(args[i]);
            }
        }
    }

    template <class T>
    static void
    _FillArbitraryArgs(std::false_type,
                       std::vector<SdfPredicateExpression::FnArg> const &,
                       std::vector<bool> const &,
                       T const &) {
        // Do nothing.
    }

    template <class ParamsTuple>
    static constexpr bool
    _TakesArbitraryArgs(std::true_type) { // arity >= 2.
        return std::is_same<
            std::tuple_element_t<std::tuple_size<ParamsTuple>::value-1,
                                 ParamsTuple>,
            std::vector<SdfPredicateExpression::FnArg>
            >::value;
    }
    
    template <class ParamsTuple>
    static constexpr bool
    _TakesArbitraryArgs(std::false_type) { // arity < 2.
        return false;
    }            

    template <class Fn>
    static PredicateFunction
    _TryToBindCall(Fn const &fn,
                   std::vector<SdfPredicateExpression::FnArg> const &args,
                   NamesAndDefaults const &namesAndDefaults) {

        // We need to determine an argument for each parameter of Fn, then make
        // a callable object that calls that function.

        // Strip DomainType arg...
        using Traits = TfFunctionTraits<Fn>;
        using FullParams = typename Traits::ArgTypes;
        using Params =
            TfMetaApply<TfMetaDecay, TfMetaApply<TfMetaTail, FullParams>>;
        using ParamsTuple = TfMetaApply<std::tuple, Params>;

        // If there are at least two parameters to Fn (first has to be
        // DomainType) and the last parameter type is vector<FnArg>, then
        // namesAndDefaults does not apply to it, and any remaining unbound args
        // after binding are passed through that parameter.
        static const bool TakesArbitraryArgs =
            _TakesArbitraryArgs<ParamsTuple>(
                std::integral_constant<bool, Traits::Arity >= 2> {});
        
        size_t minArgs = Traits::Arity-1 - namesAndDefaults.GetNumDefaults();
        size_t maxArgs = TakesArbitraryArgs ? size_t(-1) : Traits::Arity-1;

        // Number of bindable args is arity-1 (for the domain arg) or -2 if the
        // trailing parameter is the vector<FnArg> bag of extra arguments.
        static const size_t NumBindableArgs =
            Traits::Arity - (TakesArbitraryArgs ? 2 : 1);

        if (args.size() < minArgs) {
            TF_RUNTIME_ERROR("Function requires at least %zu argument%s, "
                             "%zu given", minArgs, minArgs == 1 ? "" : "s",
                             args.size());
            return {};
        }
        if (args.size() > maxArgs) {
            TF_RUNTIME_ERROR("Function takes at most %zu argument%s, %zu given",
                             maxArgs, maxArgs == 1 ? "" : "s", args.size());
            return {};
        }

        ParamsTuple typedArgs;
        std::vector<bool> boundArgs;
        if (_TryBindArgs(typedArgs, args, namesAndDefaults,
                         std::make_index_sequence<NumBindableArgs> {},
                         boundArgs)) {
            _FillArbitraryArgs(
                std::integral_constant<bool, TakesArbitraryArgs> {},
                args, boundArgs, typedArgs);
            return [typedArgs, fn](DomainType const &obj) {
                return SdfPredicateFunctionResult {
                    std::apply(fn, 
                        std::tuple_cat(std::make_tuple(obj), typedArgs))
                };
            };
        }
        return {};
    }

    struct _OverloadBinderBase
    {
        virtual ~_OverloadBinderBase() = default;
        PredicateFunction
        Bind(std::vector<SdfPredicateExpression::FnArg> const &args) const {
            return _Bind(args);
        }
        virtual std::unique_ptr<_OverloadBinderBase> Clone() const = 0;
    protected:
        _OverloadBinderBase() = default;
        
        explicit _OverloadBinderBase(NamesAndDefaults const &namesAndDefaults)
            : _namesAndDefaults(namesAndDefaults) {}

        virtual PredicateFunction
        _Bind(std::vector<
              SdfPredicateExpression::FnArg> const &args) const = 0;

        NamesAndDefaults _namesAndDefaults;
    };
    
    template <class Fn>
    struct _OverloadBinder : _OverloadBinderBase
    {
        ~_OverloadBinder() override = default;

        static std::unique_ptr<_OverloadBinder>
        TryCreate(Fn &&fn, NamesAndDefaults const &nd) {
            auto ret = std::unique_ptr<_OverloadBinder>(
                new _OverloadBinder(std::move(fn), nd));
            if (!_CheckNamesAndDefaultsWithSignature<Fn>(nd)) {
                ret.reset();
            }
            return ret;
        }

        std::unique_ptr<_OverloadBinderBase> Clone() const override {
            return std::unique_ptr<
                _OverloadBinder>(new _OverloadBinder(*this));
        }
        
    private:
        _OverloadBinder(_OverloadBinder const &) = default;
                                    
        explicit _OverloadBinder(Fn &&fn,
                                 NamesAndDefaults const &namesAndDefaults)
            : _OverloadBinderBase(namesAndDefaults)
            , _fn(std::move(fn)) {}

        explicit _OverloadBinder(Fn const &fn,
                                 NamesAndDefaults const &namesAndDefaults)
            : _OverloadBinder(Fn(fn), namesAndDefaults) {}

        PredicateFunction
        _Bind(std::vector<
              SdfPredicateExpression::FnArg> const &args) const override {
            // Try to bind 'args' to _fn's parameters, taking _namesAndDefaults
            // into account.
            return _TryToBindCall(_fn, args, this->_namesAndDefaults);
        }

        Fn _fn;
    };

    template <class Fn>
    struct _CustomBinder : _OverloadBinderBase
    {
        ~_CustomBinder() override = default;

        static std::unique_ptr<_CustomBinder>
        Create(Fn &&fn) {
            return std::unique_ptr<_CustomBinder>(
                new _CustomBinder(std::move(fn)));
        }

        std::unique_ptr<_OverloadBinderBase> Clone() const override {
            return std::unique_ptr<_CustomBinder>(new _CustomBinder(*this));
        }
        
    private:
        _CustomBinder(_CustomBinder const &) = default;
        explicit _CustomBinder(Fn &&fn)
            : _OverloadBinderBase()
            , _fn(std::move(fn)) {}
        explicit _CustomBinder(Fn const &fn) : _CustomBinder(Fn(fn)) {}

        PredicateFunction
        _Bind(std::vector<
              SdfPredicateExpression::FnArg> const &args) const override {
            // Call _fn to try to bind 'args', producing a callable.
            return _fn(args);
        }

        Fn _fn;
    };

    using _OverloadBinderBasePtr = std::unique_ptr<_OverloadBinderBase>;
    
    pxr_tsl::robin_map<
        std::string, std::vector<_OverloadBinderBasePtr>
        > _binders;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PREDICATE_EXPRESSION_EVAL_H
