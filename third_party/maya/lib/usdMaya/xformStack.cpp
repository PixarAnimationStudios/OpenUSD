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

TF_DEFINE_PUBLIC_TOKENS(PxrUsdMayaXformStackTokens,
        PXRUSDMAYA_XFORM_STACK_TOKENS);

typedef PxrUsdMayaXformStack::OpClassList OpClassList;
typedef PxrUsdMayaXformStack::OpClass OpClass;
typedef PxrUsdMayaXformStack::OpClassPair OpClassPair;

typedef PxrUsdMayaXformStack::IndexPair IndexPair;
typedef PxrUsdMayaXformStack::TokenIndexPairMap TokenIndexPairMap;
typedef PxrUsdMayaXformStack::IndexMap IndexMap;

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

    IndexMap _buildInversionMap(
            const std::vector<IndexPair>& inversionTwins)
    {
        IndexMap result;
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
    IndexPair
    _makeInversionIndexPair(
            size_t opIndex,
            const IndexMap& inversionMap)
    {
        auto foundTwin = inversionMap.find(opIndex);
        if (foundTwin == inversionMap.end())
        {
            return std::make_pair(opIndex, PxrUsdMayaXformStack::NO_INDEX);
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

    TokenIndexPairMap _buildAttrNamesToIdxs(
            const OpClassList& ops,
            const IndexMap& inversionMap)
    {
        TokenIndexPairMap result;
        for (size_t i = 0; i < ops.size(); ++i)
        {
            const PxrUsdMayaXformOpClassification& op = ops[i];
            // Inverted twin pairs will always have same name, so can skip one
            if (op.IsInvertedTwin()) continue;

            for (auto attrName : op.CompatibleAttrNames())
            {
                auto indexPair = _makeInversionIndexPair(i, inversionMap);
                // Insert, and check if it already existed
                TF_VERIFY(result.insert({attrName, indexPair}).second,
                        "AttrName %s already found in attrName lookup map",
                        attrName.GetText());
            }
        }
        return result;
    }

    TokenIndexPairMap _buildOpNamesToIdxs(
            const OpClassList& ops,
            const IndexMap& inversionMap)
    {
        TokenIndexPairMap result;
        for (size_t i = 0; i < ops.size(); ++i)
        {
            const PxrUsdMayaXformOpClassification& op = ops[i];
            // Inverted twin pairs will always have same name, so can skip one
            if (op.IsInvertedTwin()) continue;

            auto indexPair = _makeInversionIndexPair(i, inversionMap);
            // Insert, and check if it already existed
            TF_VERIFY(result.insert({op.GetName(), indexPair}).second,
                    "Op classification name %s already found in op lookup map",
                    op.GetName().GetText());
        }
        return result;
    }

}

class _PxrUsdMayaXformOpClassificationData : public TfRefBase
{
public:
    typedef TfRefPtr<_PxrUsdMayaXformOpClassificationData> RefPtr;

    RefPtr static Create(const TfToken &name,
            UsdGeomXformOp::Type opType,
            bool isInvertedTwin)
    {
        return RefPtr(new _PxrUsdMayaXformOpClassificationData(
                name, opType, isInvertedTwin));
    }

    bool operator ==(const _PxrUsdMayaXformOpClassificationData& other) const
    {
        return (m_name == other.m_name
                && m_opType == other.m_opType
                && m_isInvertedTwin == other.m_isInvertedTwin);
    }

    const TfToken m_name;
    const UsdGeomXformOp::Type m_opType;
    const bool m_isInvertedTwin;

private:
    _PxrUsdMayaXformOpClassificationData(const TfToken &name,
            UsdGeomXformOp::Type opType,
            bool isInvertedTwin) :
                    m_name(name),
                    m_opType(opType),
                    m_isInvertedTwin(isInvertedTwin)
    {
    }
};

PxrUsdMayaXformOpClassification::PxrUsdMayaXformOpClassification() :
                _sharedData(nullptr)
{
}

PxrUsdMayaXformOpClassification::PxrUsdMayaXformOpClassification(
        const TfToken &name,
        UsdGeomXformOp::Type opType,
        bool isInvertedTwin) :
                _sharedData(_PxrUsdMayaXformOpClassificationData::Create(
                        name, opType, isInvertedTwin))
{
}

PxrUsdMayaXformOpClassification const &
PxrUsdMayaXformOpClassification::NullInstance()
{
    static PxrUsdMayaXformOpClassification theNull;
    return theNull;
}

bool
PxrUsdMayaXformOpClassification::IsNull() const
{
    return !_sharedData;
}

TfToken const &
PxrUsdMayaXformOpClassification::GetName() const
{
    return _sharedData->m_name;
}

UsdGeomXformOp::Type
PxrUsdMayaXformOpClassification::GetOpType() const
{
    return _sharedData->m_opType;
}

bool
PxrUsdMayaXformOpClassification::IsInvertedTwin() const
{
    return _sharedData->m_isInvertedTwin;
}

std::vector<TfToken>
PxrUsdMayaXformOpClassification::CompatibleAttrNames() const
{
    // Note that we make tokens immortal because PxrUsdMayaXformOpClassification
    // is currently not publically creatable, and is only used to make
    // some global constants (ie, MayaStack and CommonStack)
    std::vector<TfToken> result;
    std::string attrName;
    if (_isThreeAxisRotate(GetOpType()))
    {
        // Special handling for rotates, to deal with rotateX/rotateZXY/etc
        if (GetName() == PxrUsdMayaXformStackTokens->rotate)
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
                                PxrUsdMayaXformStackTokens->rotate).GetString(),
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
PxrUsdMayaXformOpClassification::IsCompatibleType(UsdGeomXformOp::Type otherType) const
{
    if (GetOpType() == otherType) return true;
    if (_isThreeAxisRotate(GetOpType() ))
    {
        return _isOneOrThreeAxisRotate(otherType);
    }
    return false;
}

bool PxrUsdMayaXformOpClassification::operator ==(const PxrUsdMayaXformOpClassification& other) const
{
    return *_sharedData == *other._sharedData;
}

// Lame that we need this... I had thought that constexpr would essentially be treated like
// a compile-time #define, with better type safety! Instead, it seems it still creates a full-fledged
// linker symbol...
constexpr size_t PxrUsdMayaXformStack::NO_INDEX;

class _PxrUsdMayaXformStackData : public TfRefBase
{
public:
    typedef TfRefPtr<_PxrUsdMayaXformStackData> RefPtr;

    RefPtr static Create(const OpClassList& ops,
            const std::vector<IndexPair>& inversionTwins,
            bool nameMatters)
    {
        return RefPtr(new _PxrUsdMayaXformStackData(
                ops, inversionTwins, nameMatters));
    }

    _PxrUsdMayaXformStackData(
            const OpClassList& ops,
            const std::vector<IndexPair>& inversionTwins,
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
            const PxrUsdMayaXformOpClassification& first = m_ops[pair.first];
            const PxrUsdMayaXformOpClassification& second = m_ops[pair.second];
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

    inline const OpClass&
    GetOpClassFromIndex(
            const size_t i) const
    {
        return i == PxrUsdMayaXformStack::NO_INDEX ? OpClass::NullInstance() : m_ops[i];
    }

    inline OpClassPair
    MakeOpClassPairFromIndexPair(
            const IndexPair& indexPair) const
    {
        return std::make_pair(
                GetOpClassFromIndex(indexPair.first),
                GetOpClassFromIndex(indexPair.second));
    }

    const OpClassList m_ops;
    std::vector<IndexPair> m_inversionTwins;
    IndexMap m_inversionMap;

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
    TokenIndexPairMap m_attrNamesToIdxs;

    // Also have a lookup by op name, for use by FindOp
    TokenIndexPairMap m_opNamesToIdxs;

    bool m_nameMatters = true;
};

PxrUsdMayaXformStack::PxrUsdMayaXformStack(
        const OpClassList& ops,
        const std::vector<IndexPair>& inversionTwins,
        bool nameMatters) :
        _sharedData(_PxrUsdMayaXformStackData::Create(
                        ops, inversionTwins, nameMatters))
{
}

OpClassList const &
PxrUsdMayaXformStack::GetOps() const {
    return _sharedData->m_ops;
}

std::vector<IndexPair> const &
PxrUsdMayaXformStack::GetInversionTwins() const {
    return _sharedData->m_inversionTwins;
}

bool
PxrUsdMayaXformStack::GetNameMatters() const {
    return _sharedData->m_nameMatters;
}

PxrUsdMayaXformOpClassification const &
PxrUsdMayaXformStack::operator[] (const size_t index) const {
    return _sharedData->m_ops[index];
}

size_t
PxrUsdMayaXformStack::GetSize() const {
    return _sharedData->m_ops.size();
}

size_t
PxrUsdMayaXformStack::FindOpIndex(const TfToken& opName, bool isInvertedTwin) const
{
    const IndexPair& foundIdxPair = FindOpIndexPair(opName);

    if(foundIdxPair.first == NO_INDEX) return NO_INDEX;

    // we (potentially) found a pair of opPtrs... use the one that
    // matches isInvertedTwin
    const PxrUsdMayaXformOpClassification& firstOp = GetOps()[foundIdxPair.first];
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

const OpClass&
PxrUsdMayaXformStack::FindOp(const TfToken& opName, bool isInvertedTwin) const
{
    return _sharedData->GetOpClassFromIndex(FindOpIndex(opName, isInvertedTwin));
}

const IndexPair&
PxrUsdMayaXformStack::FindOpIndexPair(const TfToken& opName) const
{
    static IndexPair _NO_MATCH = std::make_pair(NO_INDEX, NO_INDEX);
    TokenIndexPairMap::const_iterator foundTokenIdxPair =
            _sharedData->m_opNamesToIdxs.find(opName);
    if (foundTokenIdxPair == _sharedData->m_opNamesToIdxs.end())
    {
        // Couldn't find the xformop in our stack, abort
        return _NO_MATCH;
    }
    return foundTokenIdxPair->second;
}

const OpClassPair
PxrUsdMayaXformStack::FindOpPair(const TfToken& opName) const
{
    return _sharedData->MakeOpClassPairFromIndexPair(FindOpIndexPair(opName));
}

OpClassList
PxrUsdMayaXformStack::MatchingSubstack(
        const std::vector<UsdGeomXformOp>& xformops) const
{
    static const OpClassList _NO_MATCH;

    if (xformops.empty()) return _NO_MATCH;

    OpClassList ret;

    // nextOp keeps track of where we will start looking for matches.  It
    // will only move forward.
    size_t nextOpIndex = 0;

    std::vector<bool> opNamesFound(GetOps().size(), false);

    TF_FOR_ALL(iter, xformops) {
        const UsdGeomXformOp& xformOp = *iter;
        size_t foundOpIdx = NO_INDEX;

        if(GetNameMatters()) {
            // First try the fast attrName lookup...
            TokenIndexPairMap::const_iterator foundTokenIdxPair =
                    _sharedData->m_attrNamesToIdxs.find(xformOp.GetName());
            if (foundTokenIdxPair == _sharedData->m_attrNamesToIdxs.end())
            {
                // Couldn't find the xformop in our stack, abort
                return _NO_MATCH;
            }

            // we found a pair of opPtrs... make sure one is
            // not less than nextOp...
            const IndexPair& foundIdxPair = foundTokenIdxPair->second;

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
        const PxrUsdMayaXformOpClassification& foundOp = GetOps()[foundOpIdx];

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

OpClassList
PxrUsdMayaXformStack::FirstMatchingSubstack(
        const std::vector<PxrUsdMayaXformStack const *>& stacks,
        const std::vector<UsdGeomXformOp>& xformops)
{
    if (xformops.empty() || stacks.empty()) return OpClassList();

    for (auto& stackPtr : stacks)
    {
        std::vector<PxrUsdMayaXformStack::OpClass> stackOps = \
                stackPtr->MatchingSubstack(xformops);
        if (!stackOps.empty())
        {
            return stackOps;
        }
    }

    return OpClassList();
}

const PxrUsdMayaXformStack&
PxrUsdMayaXformStack::MayaStack()
{
    static PxrUsdMayaXformStack mayaStack(
            // ops
            {
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->translate,
                        UsdGeomXformOp::TypeTranslate),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->rotatePivotTranslate,
                        UsdGeomXformOp::TypeTranslate),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->rotatePivot,
                        UsdGeomXformOp::TypeTranslate),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->rotate,
                        UsdGeomXformOp::TypeRotateXYZ),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->rotateAxis,
                        UsdGeomXformOp::TypeRotateXYZ),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->rotatePivot,
                        UsdGeomXformOp::TypeTranslate,
                        true /* isInvertedTwin */),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->scalePivotTranslate,
                        UsdGeomXformOp::TypeTranslate),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->scalePivot,
                        UsdGeomXformOp::TypeTranslate),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->shear,
                        UsdGeomXformOp::TypeTransform),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->scale,
                        UsdGeomXformOp::TypeScale),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->scalePivot,
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

const PxrUsdMayaXformStack&
PxrUsdMayaXformStack::CommonStack()
{
    static PxrUsdMayaXformStack commonStack(
            // ops
            {
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->translate,
                        UsdGeomXformOp::TypeTranslate),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->pivot,
                        UsdGeomXformOp::TypeTranslate),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->rotate,
                        UsdGeomXformOp::TypeRotateXYZ),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->scale,
                        UsdGeomXformOp::TypeScale),
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->pivot,
                        UsdGeomXformOp::TypeTranslate,
                        true /* isInvertedTwin */)
            },

            // inversionTwins
            {
                {1, 4},
            });

    return commonStack;
}

const PxrUsdMayaXformStack&
PxrUsdMayaXformStack::MatrixStack()
{
    static PxrUsdMayaXformStack matrixStack(
            // ops
            {
                PxrUsdMayaXformOpClassification(
                        PxrUsdMayaXformStackTokens->transform,
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
