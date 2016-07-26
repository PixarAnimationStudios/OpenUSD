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
#include "pxr/base/tf/patternMatcher.h"
#include "pxr/base/tf/stringUtils.h"


using namespace std;




TfPatternMatcher::TfPatternMatcher() :
    _caseSensitive( false ),
    _invalidReason( "uncompiled pattern" ),
    _isGlob( false ),
    _recompile( true )
{
}


TfPatternMatcher::TfPatternMatcher( const string &pattern,
                                    bool caseSensitive, bool isGlob ) :
    _caseSensitive( caseSensitive ),
    _invalidReason( "uncompiled pattern" ),
    _isGlob( isGlob ),
    _pattern( pattern ),
    _recompile( true )
{   
}


TfPatternMatcher::~TfPatternMatcher()
{
    _CleanUp();
}


bool
TfPatternMatcher::IsValid() const
{
    _Compile();
    return _invalidReason.empty();
}

bool
TfPatternMatcher::Match( const string &query, string *errorMsg ) const
{
    bool matches = false;
    if( IsValid() ) {
        int result = regexec( &_regex, query.c_str(), 0, NULL, 0 );
        if( (result == 0) || (result == REG_NOMATCH) ) {
            matches = (result == 0);
            if( errorMsg ) {
                errorMsg->clear();
            }
        } else {
            if( errorMsg ) {
                *errorMsg = _GetRegErrorMessage( result );
            }
        }
    } else {
        if( errorMsg ) {
            *errorMsg = GetInvalidReason();
        }
    }
    return matches;
}


void
TfPatternMatcher::SetIsCaseSensitive( bool sensitive )
{
    if( sensitive != _caseSensitive ) {
        _recompile = true;
        _caseSensitive = sensitive;
    }
}

void
TfPatternMatcher::SetIsGlobPattern( bool isGlob )
{
    if( isGlob != _isGlob ) {
        _recompile = true;
        _isGlob = isGlob;
    }
}


void
TfPatternMatcher::SetPattern( const string &pattern )
{
    if( pattern != _pattern ) {
        _recompile = true;
        _pattern = pattern;
    }
}





////////////////////////////////// Private ////////////////////////////////

void
TfPatternMatcher::_CleanUp() const {
    if( _invalidReason.empty() ) {
        regfree( &_regex );
        _invalidReason = "uncompiled pattern";
    }
}


void
TfPatternMatcher::_Compile() const
{
    if( _recompile ) {

        _recompile = false;

        _CleanUp();

        _invalidReason.clear();
        
        // According to POSIX, empty patterns are not valid.  This test is here
        // because some implemenations (Linux) are a bit sloppy about this.
        if( _pattern.empty() ) {
            _invalidReason = "empty (sub)expression";
            return;
        }

        string pattern = _ConvertPatternToRegex();

        int flags =
            REG_EXTENDED | REG_NEWLINE | (IsCaseSensitive() ? 0 : REG_ICASE);
        int result = regcomp( &_regex, pattern.c_str(), flags );
        if( result != 0 ) {
            _invalidReason = _GetRegErrorMessage( result );
            if( _invalidReason.empty() ) {
                // CODE_COVERAGE_OFF - this should never happen
                _invalidReason = "unknown reason";
                // CODE_COVERAGE_ON
            }
            return;
        }

    }
}


string
TfPatternMatcher::_ConvertPatternToRegex() const
{
    return IsGlobPattern() ? TfStringGlobToRegex(_pattern) : _pattern;
}


string
TfPatternMatcher::_GetRegErrorMessage( int code ) const
{
    string ret;
    if( code != 0 ) {
        char error[256];
        error[0] = '\0';
        regerror( code, &_regex, error, sizeof(error)/sizeof(error[0]) );
        ret = error;
    }
    return ret;
}


