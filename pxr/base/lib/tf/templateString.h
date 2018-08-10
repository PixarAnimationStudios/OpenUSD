//
// Copyright 2016 Pixar
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
#ifndef TF_TEMPLATE_STRING_H
#define TF_TEMPLATE_STRING_H

/// \file tf/templateString.h

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"

#include <tbb/spin_mutex.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfTemplateString
/// \ingroup group_tf_String
///
/// TfTemplateString provides simple string substitutions based on named
/// placeholders. Instead of the '%'-based substitutions used by printf,
/// template strings use '$'-based substitutions, using the following rules:
///
/// \li "$$" is replaced with a single "$"
/// \li "$identifier" names a substitution placeholder matching a mapping key
/// of "identifier". The first non-identifier character after the "$"
/// character terminates the placeholder specification.
/// \li "${identifier}" is equivalent to "$identifier". It is required when
/// valid identifier characters follow the placeholder but are not part of the
/// placeholder, such as "${noun}ification".
/// \li An identifier is a sequence of characters "[A-Z][a-z][0-9]_".
///
/// \a TfTemplateString is immutable: once one is created it may not be
/// modified.  \a TfTemplateString is fast to copy, since it shares state
/// internally between copies.  \a TfTemplateString is thread-safe.  It may be
/// read freely by multiple threads concurrently.
///
class TfTemplateString {
public:
    typedef std::map<std::string, std::string> Mapping;

    /// Constructs a new template string.
    TF_API
    TfTemplateString();

    /// Constructs a new template string.
    TF_API
    TfTemplateString(const std::string& template_);

    /// Returns the template source string supplied to the constructor.
    const std::string& GetTemplate() const { return _data->template_; }

    /// Performs the template substitution, returning a new string. The mapping
    /// contains keys which match the placeholders in the template. If a
    /// placeholder is found for which no mapping is present, a coding error is
    /// raised.
    TF_API
    std::string Substitute(const Mapping&) const;

    /// Like Substitute(), except that if placeholders are missing from the
    /// mapping, instead of raising a coding error, the original placeholder
    /// will appear in the resulting string intact.
    TF_API
    std::string SafeSubstitute(const Mapping&) const;

    /// Returns an empty mapping for the current template. This method first
    /// calls IsValid to ensure that the template is valid.
    TF_API
    Mapping GetEmptyMapping() const;

    /// Returns true if the current template is well formed. Empty templates are
    /// valid.
    TF_API
    bool IsValid() const;

    /// Returns any error messages generated during template parsing.
    TF_API
    std::vector<std::string> GetParseErrors() const;

private:
    struct _PlaceHolder {
        _PlaceHolder(const std::string& n, size_t p, size_t l)
            : name(n), pos(p), len(l) {}
        std::string name;
        size_t pos;
        size_t len;
    };

    // Returns a new string constructed by performing token replacement.
    std::string _Evaluate(const Mapping&, std::vector<std::string>* = 0) const;

    // Finds the next token-like substring in the template and returns the
    // token name, position and length. For token ${foo}, the name is 'foo',
    // position is the position of '$' and length is the length of the entire
    // token including optional enclosing braces.
    bool _FindNextPlaceHolder(size_t*, std::vector<std::string>*) const;

    // Find all tokens in the template string and store away naming and
    // position information.
    void _ParseTemplate() const;

    // Emit any parse errors as TF_CODING_ERRORs.
    void _EmitParseErrors() const;

    // Structure side-allocated and shared between copies.
    struct _Data
    {
        _Data(_Data const &) = delete;
        _Data &operator=(_Data const &) = delete;

        _Data() : parsed(false) {}

        std::string template_;
        mutable std::vector<_PlaceHolder> placeholders;
        mutable bool parsed;
        mutable std::vector<std::string> parseErrors;
        mutable tbb::spin_mutex mutex;
    };

    std::shared_ptr<_Data> _data;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_TEMPLATE_STRING_H 
