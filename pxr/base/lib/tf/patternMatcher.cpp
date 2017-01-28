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

#include "pxr/pxr.h"
#include "pxr/base/tf/patternMatcher.h"

using namespace std;

PXR_NAMESPACE_OPEN_SCOPE

TfPatternMatcher::TfPatternMatcher() :
    _caseSensitive( false ),
    _isGlob( false ),
    _recompile( true )
{
}


TfPatternMatcher::TfPatternMatcher( const string &pattern,
                                    bool caseSensitive, bool isGlob ) :
    _caseSensitive( caseSensitive ),
    _isGlob( isGlob ),
    _pattern( pattern ),
    _recompile( true )
{   
}


TfPatternMatcher::~TfPatternMatcher()
{
    // Do nothing.
}


string
TfPatternMatcher::GetInvalidReason() const
{
    _Compile();
    return _regex.GetError();
}

bool
TfPatternMatcher::IsValid() const
{
    _Compile();
    return static_cast<bool>(_regex);
}

bool
TfPatternMatcher::Match( const string &query, string *errorMsg ) const
{
    if( IsValid() ) {
        if( errorMsg ) {
            errorMsg->clear();
        }

        return _regex.Match(query);
    } else {
        if( errorMsg ) {
            *errorMsg = _regex.GetError();
        }
        return false;
    }
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
TfPatternMatcher::_Compile() const
{
    if( _recompile ) {

        _recompile = false;

        const auto flags =
            (IsCaseSensitive() ? 0 : ArchRegex::CASE_INSENSITIVE) |
            (IsGlobPattern()   ? ArchRegex::GLOB : 0);

        _regex = ArchRegex(_pattern, flags);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
