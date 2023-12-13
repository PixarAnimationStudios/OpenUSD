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

#ifndef __PXR_BASE_TRACE_H__
#define __PXR_BASE_TRACE_H__

/// \file Trace/Trace.h

// trace
#include <pxr/base/trace/api.h>
// #include <pxr/base/trace/collector.h>

// #include <pxr/base/trace/traceImpl.h>

#include <pxr/base/trace/concurrentList.h>

#include <pxr/base/trace/stringHash.h>
#include <pxr/base/trace/category.h>
#include <pxr/base/trace/staticKeyData.h>
#include <pxr/base/trace/key.h>

// #include <pxr/base/trace/dataBuffer.h>
#include <pxr/base/trace/dynamicKey.h>

// #include <pxr/base/trace/event.h>
// #include <pxr/base/trace/eventContainer.h>

// #include <pxr/base/trace/eventData.h>
// #include <pxr/base/trace/eventNode.h>

// #include <pxr/base/trace/eventList.h>
#include <pxr/base/trace/threads.h>

// #include <pxr/base/trace/aggregateNode.h>

// #include <pxr/base/trace/eventTree.h>

#include <pxr/base/trace/collection.h>
#include <pxr/base/trace/serialization.h>
#include <pxr/base/trace/collectionNotice.h>
#include <pxr/base/trace/reporterDataSourceBase.h>

// #include <pxr/base/trace/reporterBase.h>

// #include <pxr/base/trace/aggregateTree.h>
// #include <pxr/base/trace/aggregateTreeBuilder.h>

#include <pxr/base/trace/counterAccumulator.h>

// #include <pxr/base/trace/eventTreeBuilder.h>
#include <pxr/base/trace/jsonSerialization.h>

// #include <pxr/base/trace/reporter.h>

#include <pxr/base/trace/reporterDataSourceCollection.h>
// #include <pxr/base/trace/reporterDataSourceCollector.h>

#endif // __PXR_BASE_TRACE_H__
