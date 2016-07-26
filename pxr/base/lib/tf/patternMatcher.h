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
#ifndef TF_PATTERNMATCHER_H
#define TF_PATTERNMATCHER_H


#include <boost/noncopyable.hpp>

#include <sys/types.h>
#include <regex.h>
#include <string>


/*!
 * \file patternMatcher.h
 * \ingroup group_tf_String
 * \brief A simple glob and regex matching utility.
 */



/*!
 * \class TfPatternMatcher PatternMatcher.h pxr/base/tf/patternMatcher.h
 * \ingroup group_tf_String
 * \brief Class for matching regular expressions.  A matcher is good to use
 * when you have many strings to match against one pattern.  This is because
 * the matcher will only compile the regular expression once.
 */
class TfPatternMatcher : public boost::noncopyable {

  public:

    /*!
     * \brief Construct an empty (invalid) TfPatternMatcher.
     */
    TfPatternMatcher();

    /*!
     * \brief Construct a TfPatternMatcher with a default configuration.  Note
     * that pattern compilation will not occur until the first call to \a
     * Match() or \a IsValid().
     */
    TfPatternMatcher( const std::string &pattern,
                      bool caseSensitive = false,
                      bool isGlob = false );
    /*!
     * \brief Destructor
     */
    ~TfPatternMatcher();

    /*!
     * \brief If \a IsValid() returns true, this will return the reason why (if
     * any).
     */
    std::string GetInvalidReason() const {
        return _invalidReason;
    }

    /*!
     * \brief Returns true if the matcher has been set to be case sensitive,
     * false otherwise.
     */
    bool IsCaseSensitive() const {
        return _caseSensitive;
    }

    /*!
     * \brief Returns true if the matcher has been set to treat patterns as
     * glob patterns, false otherwise.
     */
    bool IsGlobPattern() const {
        return _isGlob;
    }

    /*!
     * \brief Returns true if the matcher has a valid pattern.  Note that empty
     * patterns are considered invalid.  This will cause a compile of the
     * pattern if it is not already compiled.
     */
    bool IsValid() const;

    /*!
     * \brief Returns true if \a query matches the matcher's pattern.  If there
     * is an error in matching and errorMsg is not NULL, it will be set with
     * the error message.  If the matcher is not valid, this will return false.
     * Note that this will cause a compile of the matcher's pattern if it was
     * not already compiled.
     */
    bool Match( const std::string &query, std::string *errorMsg = NULL ) const;

    /*!
     * \brief Set this matcher to match case-sensitively or not.
     */
    void SetIsCaseSensitive( bool sensitive );

    /*!
     * \brief Set this matcher to treat its pattern as a glob pattern.
     * Currently, this means that the pattern will be transformed by replacing
     * all instances of '.' with '\.', '*' with '.*', and '?' with '.', in that
     * order before being compiled as a normal regular expression.
     */
    void SetIsGlobPattern( bool isGlob );

    /*!
     * \brief Set the pattern that this matcher will use to match against.
     */
    void SetPattern( const std::string &pattern );
    
  private:

    void _CleanUp() const;
    void _Compile() const;
    std::string _ConvertPatternToRegex() const;
    std::string _GetRegErrorMessage( int code ) const;

    bool _caseSensitive;
    mutable std::string _invalidReason;
    bool _isGlob;
    std::string _pattern;
    mutable bool _recompile;
    mutable regex_t _regex;
        
};





#endif // TF_PATTERNMATCHER_H
