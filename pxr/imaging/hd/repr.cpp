//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/repr.h"
#include "pxr/base/tf/hash.h"
#include <tuple>

PXR_NAMESPACE_OPEN_SCOPE

// We use an empty token to indicate "no opinion" (i.e., a "don't care" opinion)
// which is used when compositing/reolving repr selector opinions.
// See HdReprSelector::CompositeOver.
static bool
_ReprHasOpinion(const TfToken &reprToken) {
    return !reprToken.IsEmpty();
}

bool
HdReprSelector::Contains(const TfToken &reprToken) const
{
    return (reprToken == refinedToken)
        || (reprToken == unrefinedToken)
        || (reprToken == pointsToken);
}

bool
HdReprSelector::IsActiveRepr(size_t topologyIndex) const
{
    TF_VERIFY(topologyIndex < MAX_TOPOLOGY_REPRS);
    TfToken const &reprToken = (*this)[topologyIndex];
    return !(reprToken.IsEmpty() || reprToken == HdReprTokens->disabled);
}

bool
HdReprSelector::AnyActiveRepr() const
{
    for (size_t i = 0; i < MAX_TOPOLOGY_REPRS; ++i) {
        if (IsActiveRepr(i)) {
            return true;
        }
    }
    return false;
}

HdReprSelector
HdReprSelector::CompositeOver(const HdReprSelector &under) const
{
    return HdReprSelector(
        _ReprHasOpinion(refinedToken)  ? refinedToken   : under.refinedToken,
        _ReprHasOpinion(unrefinedToken)? unrefinedToken : under.unrefinedToken,
        _ReprHasOpinion(pointsToken)   ? pointsToken    : under.pointsToken);
}

bool
HdReprSelector::operator==(const HdReprSelector &rhs) const
{
    return std::tie(refinedToken, unrefinedToken, pointsToken) ==
           std::tie(rhs.refinedToken, rhs.unrefinedToken, rhs.pointsToken);
}

bool
HdReprSelector::operator!=(const HdReprSelector &rhs) const
{
    return std::tie(refinedToken, unrefinedToken, pointsToken) !=
           std::tie(rhs.refinedToken, rhs.unrefinedToken, rhs.pointsToken);
}

bool
HdReprSelector::operator<(const HdReprSelector &rhs) const
{
    return std::tie(refinedToken, unrefinedToken, pointsToken) <
           std::tie(rhs.refinedToken, rhs.unrefinedToken, rhs.pointsToken);
}

size_t
HdReprSelector::Hash() const
{ 
    return TfHash()(*this);
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

TfToken const &
HdReprSelector::operator[](size_t topologyIndex) const
{
    switch (topologyIndex) {
        case 0: return refinedToken;
        case 1: return unrefinedToken;
        case 2: return pointsToken;
        default: return refinedToken;
    }
}

HdRepr::HdRepr() : _geomSubsetsStart(0) {};

HdRepr::~HdRepr() = default;

PXR_NAMESPACE_CLOSE_SCOPE

