//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/crateInfo.h"

#include "crateFile.h"

PXR_NAMESPACE_OPEN_SCOPE


using namespace Sdf_CrateFile;

struct SdfCrateInfo::_Impl
{
    std::unique_ptr<CrateFile> crateFile;
};

/*static*/
SdfCrateInfo
SdfCrateInfo::Open(std::string const &fileName)
{
    SdfCrateInfo result;
    if (auto newCrate = CrateFile::Open(fileName)) { 
        result._impl.reset(new _Impl);
        result._impl->crateFile = std::move(newCrate);
    }
    return result;
}

SdfCrateInfo::SummaryStats
SdfCrateInfo::GetSummaryStats() const
{
    SummaryStats stats;
    if (!*this) {
        TF_CODING_ERROR("Invalid SdfCrateInfo object");
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

vector<SdfCrateInfo::Section>
SdfCrateInfo::GetSections() const
{
    vector<Section> result;
    if (!*this) {
        TF_CODING_ERROR("Invalid SdfCrateInfo object");
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
SdfCrateInfo::GetFileVersion() const
{
    if (!*this) {
        TF_CODING_ERROR("Invalid SdfCrateInfo object");
        return TfToken();
    }
    return _impl->crateFile->GetFileVersionToken();
}

TfToken
SdfCrateInfo::GetSoftwareVersion() const
{
    return CrateFile::GetSoftwareVersionToken();
}

PXR_NAMESPACE_CLOSE_SCOPE

