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
#include "pxr/imaging/hd/repr.h"
#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE

bool
HdReprSelector::Contains(TfToken reprToken) const
{
    return (reprToken == refinedToken)
        || (reprToken == unrefinedToken)
        || (reprToken == pointsToken);
}

bool
HdReprSelector::IsEmpty() const
{
    return refinedToken.IsEmpty()
        && unrefinedToken.IsEmpty()
        && pointsToken.IsEmpty();
}

HdReprSelector
HdReprSelector::CompositeOver(const HdReprSelector &under) const
{
    return HdReprSelector(
        refinedToken.IsEmpty() ? under.refinedToken : refinedToken,
        unrefinedToken.IsEmpty() ? under.unrefinedToken : unrefinedToken,
        pointsToken.IsEmpty() ? under.pointsToken : pointsToken);
}

bool
HdReprSelector::operator==(const HdReprSelector &rhs) const
{
    return (refinedToken == rhs.refinedToken)
        && (unrefinedToken == rhs.unrefinedToken)
        && (pointsToken == rhs.pointsToken);
}

bool
HdReprSelector::operator!=(const HdReprSelector &rhs) const
{
    return (refinedToken != rhs.refinedToken)
        || (unrefinedToken != rhs.unrefinedToken)
        || (pointsToken != rhs.pointsToken);
}

bool
HdReprSelector::operator<(const HdReprSelector &rhs) const
{
    return (refinedToken < rhs.refinedToken)
        && (unrefinedToken < rhs.unrefinedToken)
        && (pointsToken < rhs.pointsToken);
}

size_t
HdReprSelector::Hash() const
{ 
    size_t hash = 0;
    boost::hash_combine(hash,
                        refinedToken);
    boost::hash_combine(hash,
                        unrefinedToken);
    boost::hash_combine(hash,
                        pointsToken);
    return hash;
}

char const*
HdReprSelector::GetText() const
{
    return refinedToken.GetText();
}

std::ostream &
operator <<(std::ostream &stream, HdReprSelector const& t)
{
    return stream << t.refinedToken
          << ", " << t.unrefinedToken
          << ", " << t.pointsToken;
}

size_t
HdReprSelector::size() const
{
    return 3;
}

TfToken const &
HdReprSelector::operator[](int index) const
{
    switch (index) {
        case 0: return refinedToken;
        case 1: return unrefinedToken;
        case 2: return pointsToken;
        default: return refinedToken;
    }
}

HdRepr::HdRepr()
{
    /*NOTHING*/
}

HdRepr::~HdRepr()
{
}

PXR_NAMESPACE_CLOSE_SCOPE

