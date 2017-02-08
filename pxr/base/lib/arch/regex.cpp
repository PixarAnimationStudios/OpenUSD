//
// Copyright 2017 Pixar
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
#include "pxr/base/arch/regex.h"
#include "pxr/base/arch/defines.h"
#if defined(ARCH_OS_WINDOWS)
#include <cstring>
#include <regex>
#else
#include <regex.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

namespace {

std::string
_Replace(std::string&& s, const std::string& from, const std::string& to)
{
    std::string::size_type pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

std::string
_GlobToRegex(std::string pattern)
{
    return _Replace(_Replace(_Replace(std::move(pattern),
                                      ".", "\\."),
                             "*", ".*" ),
                    "?", ".");
}

} // anonymous namespace

#if defined(ARCH_OS_WINDOWS)

class ArchRegex::_Impl {
public:
    _Impl(const std::string& pattern, unsigned int flags, std::string* error);
    bool Match(const char* query) const;

private:
    std::regex _regex;
};

ArchRegex::_Impl::_Impl(
    const std::string& pattern, unsigned int flags, std::string* error)
{
    auto stdflags = std::regex_constants::extended;
    stdflags |=
        std::regex_constants::nosubs |
        std::regex_constants::optimize;
    if (flags & ArchRegex::CASE_INSENSITIVE) {
        stdflags |= std::regex_constants::icase;
    }

    try {
        _regex = std::regex(pattern.c_str(), stdflags);
    }
    catch (std::regex_error& e) {
        *error = e.what();
        throw;
    }
}

bool
ArchRegex::_Impl::Match(const char* query) const
{
    std::cmatch result;

    // Don't match newlines to mirror UNIX REG_NEWLINE flag but this
    // doesn't match literal newlines in the regex so this doesn't
    // match exactly the same.
    const char* eol = strchr(query, '\n');
    while (eol) {
        if (std::regex_search(query, eol, result, _regex)) {
            return true;
        }
        query = eol + 1;
        eol   = strchr(query, '\n');
    }
    return std::regex_search(query, result, _regex);
}

#else // defined(ARCH_OS_WINDOWS)

class ArchRegex::_Impl {
public:
    _Impl(const std::string& pattern, unsigned int flags, std::string* error);
    ~_Impl();
    bool Match(const char* query) const;

private:
    regex_t _regex;
};

ArchRegex::_Impl::_Impl(
    const std::string& pattern, unsigned int flags, std::string* error)
{
    const int regflags =
        REG_EXTENDED |
        REG_NEWLINE |
        ((flags & ArchRegex::CASE_INSENSITIVE) ? REG_ICASE : 0);

    const int result = regcomp(&_regex, pattern.c_str(), regflags);
    if (result != 0) {
        char buffer[256];
        buffer[0] = '\0';
        regerror(result, &_regex, buffer, sizeof(buffer));
        *error = buffer;
        throw result;
    }
}

ArchRegex::_Impl::~_Impl()
{
    regfree(&_regex);
}

bool
ArchRegex::_Impl::Match(const char* query) const
{
    return regexec(&_regex, query, 0, NULL, 0) != REG_NOMATCH;
}

#endif // defined(ARCH_OS_WINDOWS)

ArchRegex::ArchRegex()
{
    // Do nothing.
}

ArchRegex::ArchRegex(const std::string& pattern, unsigned int flags) :
    _flags(flags)
{
    try {
        if (pattern.empty()) {
            _error = "empty pattern";
        }
        else if (flags & GLOB) {
            _impl.reset(new _Impl(_GlobToRegex(pattern), _flags, &_error));
        }
        else {
            _impl.reset(new _Impl(pattern, _flags, &_error));
        }
    }
    catch (...) {
        if (_error.empty()) {
            _error = "unknown reason";
        }
    }
}

ArchRegex::ArchRegex(ArchRegex&& rhs) :
    _flags(std::move(rhs._flags)),
    _error(std::move(rhs._error)),
    _impl (std::move(rhs._impl))
{
    // Do nothing
}

ArchRegex&
ArchRegex::operator=(ArchRegex&& rhs)
{
    _flags = std::move(rhs._flags);
    _error = std::move(rhs._error);
    _impl  = std::move(rhs._impl);
    return *this;
}

ArchRegex::~ArchRegex()
{
    // Do nothing.
}

ArchRegex::operator bool() const
{
    return static_cast<bool>(_impl);
}

std::string
ArchRegex::GetError() const
{
    return _impl ? "" : (_error.empty() ? "uncompiled pattern" : _error);
}

unsigned int
ArchRegex::GetFlags() const
{
    return _flags;
}

bool
ArchRegex::Match(const std::string& query) const
{
    return _impl && _impl->Match(query.c_str());
}

PXR_NAMESPACE_CLOSE_SCOPE
