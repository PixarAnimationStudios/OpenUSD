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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_PORTABLE_UTILS_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_PORTABLE_UTILS_H

#include <codecvt>
#include <locale>

PXR_NAMESPACE_OPEN_SCOPE

/// Converts a UTF-8 string to UTF-16 (variable-width encoding).
///
/// \note This will throw a range_error exception if the input is invalid.
inline std::wstring s2w(const std::string& utf8Source)
{
// TODO: Work out how to avoid deprecated warnings, as there is nothing to replace this in the
// standard.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#elif __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    return converter.from_bytes(utf8Source);
#if defined(_MSC_VER)
#pragma warning(pop)
#elif __clang__
#pragma clang diagnostic pop
#endif
}

/// Converts a UTF-16 string to UTF-8 (variable-width encoding).
///
/// \note This will throw a range_error exception if the input is invalid.
inline std::string w2s(const std::wstring& utf16Source)
{
// TODO: Work out how to avoid deprecated warnings, as there is nothing to replace this in the
// standard.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#elif __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    return converter.to_bytes(utf16Source);
#if defined(_MSC_VER)
#pragma warning(pop)
#elif __clang__
#pragma clang diagnostic pop
#endif
}

inline int wtoi(const wchar_t* str)
{
#if defined(WIN32)
    int returnValue = ::_wtoi(str);
#else
    long longInt = ::wcstol(str, nullptr, 10);
#endif
    if (errno == ERANGE)
    {
        return 0;
    }
    else
    {
#if !defined (WIN32)
        int returnValue = (int)longInt;
#endif
        return returnValue;
    }
}

inline double wtof(const wchar_t* str)
{
#if defined(WIN32)
    return ::_wtof(str);
#else
    return ::wcstod(str, nullptr);
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_PARSER_PORTABLE_UTILS_H
