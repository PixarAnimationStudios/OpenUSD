//
// Copyright 2018 Pixar
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

#include "pxr/base/trace/reporterBase.h"

#include "pxr/pxr.h"
#include "pxr/base/trace/collector.h"
#include "pxr/base/trace/serialization.h"

PXR_NAMESPACE_OPEN_SCOPE

TraceReporterBase::TraceReporterBase(DataSourcePtr dataSource)
    : _dataSource(std::move(dataSource))
{
}

bool TraceReporterBase::SerializeProcessedCollections(std::ostream& ostr) const
{
    std::vector<CollectionPtr> collections;
    for (const CollectionPtr& col : _processedCollections) {
        collections.push_back(col);
    }
    return TraceSerialization::Write(ostr, collections);
}

TraceReporterBase::~TraceReporterBase()
{
}

void
TraceReporterBase::_Clear()
{
    _processedCollections.clear();
    if (_dataSource) {
        _dataSource->Clear();
    }
}

void
TraceReporterBase::_Update()
{
    if (!_dataSource) return;

    std::vector<CollectionPtr> data = _dataSource->ConsumeData();
    for (const CollectionPtr& collection : data) {
        _ProcessCollection(collection);
        _processedCollections.push_back(collection);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
