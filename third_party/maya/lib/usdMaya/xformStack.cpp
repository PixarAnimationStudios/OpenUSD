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

#include "usdMaya/xformStack.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/stringUtils.h"

#include <exception>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdMayaXformStackTokens,
        PXRUSDMAYA_XFORM_STACK_TOKENS);

namespace {
    const UsdGeomXformOp::Type RotateOpTypes[] = {
        UsdGeomXformOp::TypeRotateX,
        UsdGeomXformOp::TypeRotateY,
        UsdGeomXformOp::TypeRotateZ,
        UsdGeomXformOp::TypeRotateXYZ,
        UsdGeomXformOp::TypeRotateXZY,
        UsdGeomXformOp::TypeRotateYXZ,
        UsdGeomXformOp::TypeRotateYZX,
        UsdGeomXformOp::TypeRotateZXY,
        UsdGeomXformOp::TypeRotateZYX
    };

    bool _isThreeAxisRotate(UsdGeomXformOp::Type opType)
    {
        return (opType == UsdGeomXformOp::TypeRotateXYZ
                || opType == UsdGeomXformOp::TypeRotateXZY
                || opType == UsdGeomXformOp::TypeRotateYXZ
                || opType == UsdGeomXformOp::TypeRotateYZX
                || opType == UsdGeomXformOp::TypeRotateZXY
                || opType == UsdGeomXformOp::TypeRotateZYX);
    }

    bool _isOneOrThreeAxisRotate(UsdGeomXformOp::Type opType)
    {
        return (opType == UsdGeomXformOp::TypeRotateXYZ
                || opType == UsdGeomXformOp::TypeRotateXZY
                || opType == UsdGeomXformOp::TypeRotateYXZ
                || opType == UsdGeomXformOp::TypeRotateYZX
                || opType == UsdGeomXformOp::TypeRotateZXY
                || opType == UsdGeomXformOp::TypeRotateZYX
                || opType == UsdGeomXformOp::TypeRotateX
                || opType == UsdGeomXformOp::TypeRotateY
                || opType == UsdGeomXformOp::TypeRotateZ);
    }

    UsdMayaXformStack::IndexMap
    _buildInversionMap(
            const std::vector<UsdMayaXformStack::IndexPair>& inversionTwins)
    {
        UsdMayaXformStack::IndexMap result;
        for (const auto& twinPair : inversionTwins)
        {
            result[twinPair.first] = twinPair.second;
            result[twinPair.second] = twinPair.first;
        }
        return result;
    }

    // Given a single index into ops, return the pair of
    // indices, which is:
    //     { opIndex, NO_INDEX }     if opIndex has no inversion twin
    //     { opIndex, opIndexTwin }  if opIndex has an inversion twin, and opIndex < opIndexTwin
    //     { opIndexTwin, opIndex }  if opIndex has an inversion twin, and opIndex > opIndexTwin
    UsdMayaXformStack::IndexPair
    _makeInversionIndexPair(
            size_t opIndex,
            const UsdMayaXformStack::IndexMap& inversionMap)
    {
        auto foundTwin = inversionMap.find(opIndex);
        if (foundTwin == inversionMap.end())
        {
            return std::make_pair(opIndex, UsdMayaXformStack::NO_INDEX);
        }
        else
        {
            const size_t twinOpIndex = foundTwin->second;
            if (twinOpIndex >= opIndex)
            {
                return std::make_pair(opIndex, twinOpIndex);
            }
            else
            {
                return std::make_pair(twinOpIndex, opIndex);
            }
        }
    }

    UsdMayaXformStack::TokenIndexPairMap
    _buildAttrNamesToIdxs(
            const UsdMayaXformStack::OpClassList& ops,
            const UsdMayaXformStack::IndexMap& inversionMap)
    {
        UsdMayaXformStack::TokenIndexPairMap result;
        for (size_t i = 0; i < ops.size(); ++i)
        {
            const UsdMayaXformOpClassification& op = ops[i];
            // Inverted twin pairs will always have same name, so can skip one
            if (op.IsInvertedTwin()) continue;

            for (auto attrName : op.CompatibleAttrNames())
            {
                UsdMayaXformStack::IndexPair indexPair =
                        _makeInversionIndexPair(i, inversionMap);
                // Insert, and check if it already existed
                TF_VERIFY(result.insert({attrName, indexPair}).second,
                        "AttrName %s already found in attrName lookup map",
                        attrName.GetText());
            }
        }
        return result;
    }

    UsdMayaXformStack::TokenIndexPairMap _buildOpNamesToIdxs(
            const UsdMayaXformStack::OpClassList& ops,
            const UsdMayaXformStack::IndexMap& inversionMap)
    {
        UsdMayaXformStack::TokenIndexPairMap result;
        for (size_t i = 0; i < ops.size(); ++i)
        {
            const UsdMayaXformOpClassification& op = ops[i];
            // Inverted twin pairs will always have same name, so can skip one
            if (op.IsInvertedTwin()) continue;

            UsdMayaXformStack::IndexPair indexPair =
                    _makeInversionIndexPair(i, inversionMap);
            // Insert, and check if it already existed
            TF_VERIFY(result.insert({op.GetName(), indexPair}).second,
                    "Op classification name %s already found in op lookup map",
                    op.GetName().GetText());
        }
        return result;
    }

}

class UsdMayaXformOpClassification::_Data : public TfRefBase
{
public:
    _DataRefPtr static Create(const TfToken &name,
            UsdGeomXformOp::Type opType,
            bool isInvertedTwin)
    {
        return _DataRefPtr(new _Data(name, opType, isInvertedTwin));
    }

    bool operator ==(const _Data& other) const
    {
        return (m_name == other.m_name
                && m_opType == other.m_opType
                && m_isInvertedTwin == other.m_isInvertedTwin);
    }

    const TfToken m_name;
    const UsdGeomXformOp::Type m_opType;
    const bool m_isInvertedTwin;

private:
    _Data(const TfToken &name, UsdGeomXformOp::Type opType, bool isInvertedTwin)
        : m_name(name),
          m_opType(opType),
          m_isInvertedTwin(isInvertedTwin)
    {
    }
};

UsdMayaXformOpClassification::UsdMayaXformOpClassification() :
                _sharedData(nullptr)
{
}

UsdMayaXformOpClassification::UsdMayaXformOpClassification(
        const TfToken &name,
        UsdGeomXformOp::Type opType,
        bool isInvertedTwin) :
                _sharedData(_Data::Create(name, opType, isInvertedTwin))
{
}

UsdMayaXformOpClassification const &
UsdMayaXformOpClassification::NullInstance()
{
    static UsdMayaXformOpClassification theNull;
    return theNull;
}

bool
UsdMayaXformOpClassification::IsNull() const
{
    return !_sharedData;
}

TfToken const &
UsdMayaXformOpClassification::GetName() const
{
    return _sharedData->m_name;
}

UsdGeomXformOp::Type
UsdMayaXformOpClassification::GetOpType() const
{
    return _sharedData->m_opType;
}

bool
UsdMayaXformOpClassification::IsInvertedTwin() const
{
    return _sharedData->m_isInvertedTwin;
}

std::vector<TfToken>
UsdMayaXformOpClassification::CompatibleAttrNames() const
{
    // Note that we make tokens immortal because UsdMayaXformOpClassification
    // is currently not publically creatable, and is only used to make
    // some global constants (ie, MayaStack and CommonStack)
    std::vector<TfToken> result;
    std::string attrName;
    if (_isThreeAxisRotate(GetOpType()))
    {
        // Special handling for rotates, to deal with rotateX/rotateZXY/etc
        if (GetName() == UsdMayaXformStackTokens->rotate)
        {
            result.reserve(std::extent<decltype(RotateOpTypes)>::value * 3);
            // Special handling for rotate, to deal with rotateX/rotateZXY/etc
            for (UsdGeomXformOp::Type rotateType : RotateOpTypes)
            {
                // Add, ie, xformOp::rotateX
                result.emplace_back(TfToken(
                        UsdGeomXformOp::GetOpName(rotateType).GetString(),
                        TfToken::Immortal));
                // Add, ie, xformOp::rotateX::rotate
                result.emplace_back(TfToken(
                        UsdGeomXformOp::GetOpName(rotateType,
                                UsdMayaXformStackTokens->rotate).GetString(),
                        TfToken::Immortal));
                // Add, ie, xformOp::rotateX::rotateX
                result.emplace_back(TfToken(
                        UsdGeomXformOp::GetOpName(rotateType,
                                UsdGeomXformOp::GetOpTypeToken(rotateType)).GetString(),
                        TfToken::Immortal));
            }
        }
        else
        {
            result.reserve(std::extent<decltype(RotateOpTypes)>::value);
            for (UsdGeomXformOp::Type rotateType : RotateOpTypes)
            {
                // Add, ie, xformOp::rotateX::rotateAxis
                result.emplace_back(TfToken(
                        UsdGeomXformOp::GetOpName(rotateType,
                                GetName()).GetString(),
                        TfToken::Immortal));
            }
        }
    }
    else
    {
        // Add, ie, xformOp::translate::someName
        result.emplace_back(TfToken(
                UsdGeomXformOp::GetOpName(GetOpType(), GetName()).GetString(),
                TfToken::Immortal));
        if (GetName() == UsdGeomXformOp::GetOpTypeToken(GetOpType()))
        {
            // Add, ie, xformOp::translate
            result.emplace_back(TfToken(
                    UsdGeomXformOp::GetOpName(GetOpType()).GetString(),
                    TfToken::Immortal));
        }
    }
    return result;
}

bool
UsdMayaXformOpClassification::IsCompatibleType(UsdGeomXformOp::Type otherType) const
{
    if (GetOpType() == otherType) return true;
    if (_isThreeAxisRotate(GetOpType() ))
    {
        return _isOneOrThreeAxisRotate(otherType);
    }
    return false;
}

bool UsdMayaXformOpClassification::operator ==(const UsdMayaXformOpClassification& other) const
{
    return *_sharedData == *other._sharedData;
}

// Lame that we need this... I had thought that constexpr would essentially be treated like
// a compile-time #define, with better type safety! Instead, it seems it still creates a full-fledged
// linker symbol...
constexpr size_t UsdMayaXformStack::NO_INDEX;

class UsdMayaXformStack::_Data : public TfRefBase
{
public:
    _DataRefPtr static Create(
            const UsdMayaXformStack::OpClassList& ops,
            const std::vector<UsdMayaXformStack::IndexPair>& inversionTwins,
            bool nameMatters)
    {
        return _DataRefPtr(new _Data(ops, inversionTwins, nameMatters));
    }

    _Data(
            const UsdMayaXformStack::OpClassList& ops,
            const std::vector<UsdMayaXformStack::IndexPair>& inversionTwins,
            bool nameMatters) :
                    m_ops(ops),
                    m_inversionTwins(inversionTwins),
                    m_inversionMap(_buildInversionMap(inversionTwins)),
                    m_attrNamesToIdxs(
                            _buildAttrNamesToIdxs(m_ops, m_inversionMap)),
                    m_opNamesToIdxs(
                            _buildOpNamesToIdxs(m_ops, m_inversionMap)),
                    m_nameMatters(nameMatters)
    {
        // Verify that all inversion twins are of same type, and exactly one is marked
        // as the inverted twin
        for (auto& pair : m_inversionTwins)
        {
            const UsdMayaXformOpClassification& first = m_ops[pair.first];
            const UsdMayaXformOpClassification& second = m_ops[pair.second];
            TF_VERIFY(first.GetName() == second.GetName(),
                    "Inversion twins %lu (%s) and %lu (%s) did not have same name",
                    pair.first, first.GetName().GetText(),
                    pair.second, second.GetName().GetText());
            TF_VERIFY(first.GetOpType() == second.GetOpType(),
                    "Inversion twins %lu and %lu (%s) were not same op type",
                    pair.first, pair.second, first.GetName().GetText());
            TF_VERIFY(first.IsInvertedTwin() != second.IsInvertedTwin(),
                    "Inversion twins %lu and %lu (%s) were both marked as %s inverted twin",
                    pair.first, pair.second, first.GetName().GetText(),
                    first.IsInvertedTwin() ? "the" : "not the");
        }
    }

    inline const UsdMayaXformStack::OpClass&
    GetOpClassFromIndex(
            const size_t i) const
    {
        return i == UsdMayaXformStack::NO_INDEX
                ? UsdMayaXformStack::OpClass::NullInstance()
                : m_ops[i];
    }

    inline UsdMayaXformStack::OpClassPair
    MakeOpClassPairFromIndexPair(
            const UsdMayaXformStack::IndexPair& indexPair) const
    {
        return std::make_pair(
                GetOpClassFromIndex(indexPair.first),
                GetOpClassFromIndex(indexPair.second));
    }

    const UsdMayaXformStack::OpClassList m_ops;
    std::vector<UsdMayaXformStack::IndexPair> m_inversionTwins;
    UsdMayaXformStack::IndexMap m_inversionMap;

    // We store lookups from raw attribute name - use full attribute
    // name because it's the only "piece" we know we have a pre-generated
    // TfToken for - even Property::GetBaseName() generates a new TfToken
    // "on the fly".
    // The lookup maps to a PAIR of indices into the ops list;
    // we return a pair because, due to inversion twins, it's possible
    // for there to be two (but there should be only two!) ops with
    // the same name - ie, if they're inversion twins. Thus, each pair
    // of indices will either be:
    //     { opIndex, NO_INDEX }     if opIndex has no inversion twin
    //     { opIndex, opIndexTwin }  if opIndex has an inversion twin, and opIndex < opIndexTwin
    //     { opIndexTwin, opIndex }  if opIndex has an inversion twin, and opIndex > opIndexTwin
    UsdMayaXformStack::TokenIndexPairMap m_attrNamesToIdxs;

    // Also have a lookup by op name, for use by FindOp
    UsdMayaXformStack::TokenIndexPairMap m_opNamesToIdxs;

    bool m_nameMatters = true;
};

UsdMayaXformStack::UsdMayaXformStack(
        const UsdMayaXformStack::OpClassList& ops,
        const std::vector<UsdMayaXformStack::IndexPair>& inversionTwins,
        bool nameMatters) :
        _sharedData(_Data::Create(ops, inversionTwins, nameMatters))
{
}

UsdMayaXformStack::OpClassList const &
UsdMayaXformStack::GetOps() const {
    return _sharedData->m_ops;
}

std::vector<UsdMayaXformStack::IndexPair> const &
UsdMayaXformStack::GetInversionTwins() const {
    return _sharedData->m_inversionTwins;
}

bool
UsdMayaXformStack::GetNameMatters() const {
    return _sharedData->m_nameMatters;
}

UsdMayaXformOpClassification const &
UsdMayaXformStack::operator[] (const size_t index) const {
    return _sharedData->m_ops[index];
}

size_t
UsdMayaXformStack::GetSize() const {
    return _sharedData->m_ops.size();
}

size_t
UsdMayaXformStack::FindOpIndex(const TfToken& opName, bool isInvertedTwin) const
{
    const UsdMayaXformStack::IndexPair& foundIdxPair =
            FindOpIndexPair(opName);

    if(foundIdxPair.first == NO_INDEX) return NO_INDEX;

    // we (potentially) found a pair of opPtrs... use the one that
    // matches isInvertedTwin
    const UsdMayaXformOpClassification& firstOp = GetOps()[foundIdxPair.first];
    if (firstOp.IsInvertedTwin())
    {
        if (isInvertedTwin) return foundIdxPair.first;
        else return foundIdxPair.second;
    }
    else
    {
        if (isInvertedTwin) return foundIdxPair.second;
        else return foundIdxPair.first;
    }
}

const UsdMayaXformStack::OpClass&
UsdMayaXformStack::FindOp(const TfToken& opName, bool isInvertedTwin) const
{
    return _sharedData->GetOpClassFromIndex(FindOpIndex(opName, isInvertedTwin));
}

const UsdMayaXformStack::IndexPair&
UsdMayaXformStack::FindOpIndexPair(const TfToken& opName) const
{
    static UsdMayaXformStack::IndexPair _NO_MATCH =
            std::make_pair(NO_INDEX, NO_INDEX);
    UsdMayaXformStack::TokenIndexPairMap::const_iterator foundTokenIdxPair =
            _sharedData->m_opNamesToIdxs.find(opName);
    if (foundTokenIdxPair == _sharedData->m_opNamesToIdxs.end())
    {
        // Couldn't find the xformop in our stack, abort
        return _NO_MATCH;
    }
    return foundTokenIdxPair->second;
}

const UsdMayaXformStack::OpClassPair
UsdMayaXformStack::FindOpPair(const TfToken& opName) const
{
    return _sharedData->MakeOpClassPairFromIndexPair(FindOpIndexPair(opName));
}

UsdMayaXformStack::OpClassList
UsdMayaXformStack::MatchingSubstack(
        const std::vector<UsdGeomXformOp>& xformops) const
{
    static const UsdMayaXformStack::OpClassList _NO_MATCH;

    if (xformops.empty()) return _NO_MATCH;

    UsdMayaXformStack::OpClassList ret;

    // nextOp keeps track of where we will start looking for matches.  It
    // will only move forward.
    size_t nextOpIndex = 0;

    std::vector<bool> opNamesFound(GetOps().size(), false);

    TF_FOR_ALL(iter, xformops) {
        const UsdGeomXformOp& xformOp = *iter;
        size_t foundOpIdx = NO_INDEX;

        if(GetNameMatters()) {
            // First try the fast attrName lookup...
            const auto foundTokenIdxPairIter =
                    _sharedData->m_attrNamesToIdxs.find(xformOp.GetName());
            if (foundTokenIdxPairIter == _sharedData->m_attrNamesToIdxs.end())
            {
                // Couldn't find the xformop in our stack, abort
                return _NO_MATCH;
            }

            // we found a pair of opPtrs... make sure one is
            // not less than nextOp...
            const UsdMayaXformStack::IndexPair& foundIdxPair =
                    foundTokenIdxPairIter->second;

            if (foundIdxPair.first >= nextOpIndex)
            {
                foundOpIdx = foundIdxPair.first;
            }
            else if (foundIdxPair.second >= nextOpIndex && foundIdxPair.second != NO_INDEX)
            {
                foundOpIdx = foundIdxPair.second;
            }
            else
            {
                // The result we found is before an earlier-found op,
                // so it doesn't match our stack... abort.
                return _NO_MATCH;
            }

            assert(foundOpIdx != NO_INDEX);

            // Now check that the op type matches...
            if (!GetOps()[foundOpIdx].IsCompatibleType(xformOp.GetOpType()))
            {
                return _NO_MATCH;
            }
        }
        else {
            // If name does not matter, we just iterate through the remaining ops, until
            // we find one with a matching type...
            for(size_t i = nextOpIndex; i < GetSize(); ++i)
            {
                if (GetOps()[i].IsCompatibleType(xformOp.GetOpType()))
                {
                    foundOpIdx = i;
                    break;
                }
            }
            if (foundOpIdx == NO_INDEX) return _NO_MATCH;
        }


        // Ok, we found a match...
        const UsdMayaXformOpClassification& foundOp = GetOps()[foundOpIdx];

        // move the nextOp pointer along.
        ret.push_back(foundOp);
        opNamesFound[foundOpIdx] = true;
        nextOpIndex = foundOpIdx + 1;
    }

    // check pivot pairs
    TF_FOR_ALL(pairIter, GetInversionTwins()) {
        if (opNamesFound[pairIter->first] != opNamesFound[pairIter->second]) {
            return _NO_MATCH;
        }
    }

    return ret;
}

UsdMayaXformStack::OpClassList
UsdMayaXformStack::FirstMatchingSubstack(
        const std::vector<UsdMayaXformStack const *>& stacks,
        const std::vector<UsdGeomXformOp>& xformops)
{
    if (xformops.empty() || stacks.empty()) {
        return UsdMayaXformStack::OpClassList();
    }

    for (auto& stackPtr : stacks)
    {
        std::vector<UsdMayaXformStack::OpClass> stackOps = \
                stackPtr->MatchingSubstack(xformops);
        if (!stackOps.empty())
        {
            return stackOps;
        }
    }

    return UsdMayaXformStack::OpClassList();
}

const UsdMayaXformStack&
UsdMayaXformStack::MayaStack()
{
    static UsdMayaXformStack mayaStack(
            // ops
            {
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->translate,
                        UsdGeomXformOp::TypeTranslate),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->rotatePivotTranslate,
                        UsdGeomXformOp::TypeTranslate),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->rotatePivot,
                        UsdGeomXformOp::TypeTranslate),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->rotate,
                        UsdGeomXformOp::TypeRotateXYZ),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->rotateAxis,
                        UsdGeomXformOp::TypeRotateXYZ),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->rotatePivot,
                        UsdGeomXformOp::TypeTranslate,
                        true /* isInvertedTwin */),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->scalePivotTranslate,
                        UsdGeomXformOp::TypeTranslate),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->scalePivot,
                        UsdGeomXformOp::TypeTranslate),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->shear,
                        UsdGeomXformOp::TypeTransform),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->scale,
                        UsdGeomXformOp::TypeScale),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->scalePivot,
                        UsdGeomXformOp::TypeTranslate,
                        true /* isInvertedTwin */)
            },

            // inversionTwins
            {
                {2, 5},
                {7, 10},
            });

    return mayaStack;
}

const UsdMayaXformStack&
UsdMayaXformStack::CommonStack()
{
    static UsdMayaXformStack commonStack(
            // ops
            {
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->translate,
                        UsdGeomXformOp::TypeTranslate),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->pivot,
                        UsdGeomXformOp::TypeTranslate),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->rotate,
                        UsdGeomXformOp::TypeRotateXYZ),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->scale,
                        UsdGeomXformOp::TypeScale),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->pivot,
                        UsdGeomXformOp::TypeTranslate,
                        true /* isInvertedTwin */)
            },

            // inversionTwins
            {
                {1, 4},
            });

    return commonStack;
}

const UsdMayaXformStack&
UsdMayaXformStack::MatrixStack()
{
    static UsdMayaXformStack matrixStack(
            // ops
            {
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->transform,
                        UsdGeomXformOp::TypeTransform)
            },

            // inversionTwins
            {
            },

            // nameMatters
            false
    );

    return matrixStack;
}

PXR_NAMESPACE_CLOSE_SCOPE
