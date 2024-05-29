//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/crateInfo.h"

#include "crateFile.h"

PXR_NAMESPACE_OPEN_SCOPE


using namespace Usd_CrateFile;

struct UsdCrateInfo::_Impl
{
    std::unique_ptr<CrateFile> crateFile;
};

/*static*/
UsdCrateInfo
UsdCrateInfo::Open(std::string const &fileName)
{
    UsdCrateInfo result;
    if (auto newCrate = CrateFile::Open(fileName)) { 
        result._impl.reset(new _Impl);
        result._impl->crateFile = std::move(newCrate);
    }
    return result;
}

UsdCrateInfo::SummaryStats
UsdCrateInfo::GetSummaryStats() const
{
    SummaryStats stats;
    if (!*this) {
        TF_CODING_ERROR("Invalid UsdCrateInfo object");
    }
    else {
        stats.numSpecs = _impl->crateFile->GetSpecs().size();
        stats.numUniquePaths = _impl->crateFile->GetPaths().size();
        stats.numUniqueTokens = _impl->crateFile->GetTokens().size();
        stats.numUniqueStrings = _impl->crateFile->GetStrings().size();
        stats.numUniqueFields = _impl->crateFile->GetFields().size();
        stats.numUniqueFieldSets = _impl->crateFile->GetNumUniqueFieldSets();
    }
    return stats;
}

vector<UsdCrateInfo::Section>
UsdCrateInfo::GetSections() const
{
    vector<Section> result;
    if (!*this) {
        TF_CODING_ERROR("Invalid UsdCrateInfo object");
    }
    else {
        auto secs = _impl->crateFile->GetSectionsNameStartSize();
        for (auto const &s: secs) {
            result.emplace_back(std::get<0>(s), std::get<1>(s), std::get<2>(s));
        }
    }
    return result;
}

TfToken
UsdCrateInfo::GetFileVersion() const
{
    if (!*this) {
        TF_CODING_ERROR("Invalid UsdCrateInfo object");
        return TfToken();
    }
    return _impl->crateFile->GetFileVersionToken();
}

TfToken
UsdCrateInfo::GetSoftwareVersion() const
{
    return CrateFile::GetSoftwareVersionToken();
}

PXR_NAMESPACE_CLOSE_SCOPE

