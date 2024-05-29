//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

std::ostream& operator<<(std::ostream& out,
                         const HdRenderPassAovBinding& desc)
{
    out << "RenderPassAovBinding: {"
        << desc.aovName << ", "
        << desc.renderBuffer << ", "
        << desc.renderBufferId << ", "
        << desc.clearValue << ", "
        << "aovSettings: { ";
    for (auto const& pair : desc.aovSettings) {
        out << pair.first << ": " << pair.second << ", ";
    }
    out << "}}";
    return out;
}

bool operator==(const HdRenderPassAovBinding& lhs,
                const HdRenderPassAovBinding& rhs)
{
    return lhs.aovName           == rhs.aovName           &&
           lhs.renderBuffer      == rhs.renderBuffer      &&
           lhs.renderBufferId    == rhs.renderBufferId    &&
           lhs.clearValue        == rhs.clearValue        &&
           lhs.aovSettings       == rhs.aovSettings;
}

bool operator!=(const HdRenderPassAovBinding& lhs,
                const HdRenderPassAovBinding& rhs)
{
    return !(lhs == rhs);
}

size_t hash_value(const HdRenderPassAovBinding &b) {
    return b.renderBufferId.GetHash();
}

bool HdAovHasDepthSemantic(TfToken const& aovName)
{
    // Expect depth aov's to end with (case-insensitive) "depth".
    // Because depth contains ASCII only characters, we can compare by only
    // case folding [A-Z]
    return TfStringEndsWith(
                TfStringToLowerAscii(aovName.GetString()),
                HdAovTokens->depth);
}

bool HdAovHasDepthStencilSemantic(TfToken const& aovName)
{
    // Expect depthStencil aov's to end with (case-insensitive) "depthStencil".
    // Because depthStencil contains ASCII only characters, we can compare by
    // only case folding [A-Z]
    return TfStringEndsWith(
                TfStringToLowerAscii(aovName.GetString()),
                TfStringToLowerAscii(HdAovTokens->depthStencil));
}

HdParsedAovToken::HdParsedAovToken()
    : name(), isPrimvar(false), isLpe(false), isShader(false)
{
}

HdParsedAovToken::HdParsedAovToken(TfToken const& aovName)
    : isPrimvar(false)
    , isLpe(false)
    , isShader(false)
{
    std::string const& aov = aovName.GetString();
    std::string const& primvars = HdAovTokens->primvars.GetString();
    std::string const& lpe = HdAovTokens->lpe.GetString();
    std::string const& shader = HdAovTokens->shader.GetString();

    if (aov.size() > primvars.size() &&
        aov.compare(0, primvars.size(), primvars) == 0) {
        name = TfToken(aov.substr(primvars.size()));
        isPrimvar = true;
    } else if (aov.size() > lpe.size() &&
               aov.compare(0, lpe.size(), lpe) == 0) {
        name = TfToken(aov.substr(lpe.size()));
        isLpe = true;
    } else if (aov.size() > shader.size() &&
               aov.compare(0, shader.size(), shader) == 0) {
        name = TfToken(aov.substr(shader.size()));
        isShader = true;
    } else {
        name = aovName;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
