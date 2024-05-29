//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TRACE_CATEGORY_H
#define PXR_BASE_TRACE_CATEGORY_H

/// \file trace/category.h

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/trace/stringHash.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Categories that a TraceReporter can use to filter events.
using TraceCategoryId = uint32_t;

///////////////////////////////////////////////////////////////////////////////
///
/// \class TraceCategory
///
/// This singleton class provides a way to mark TraceEvent instances with 
/// category Ids which can be used to filter them. This class also provides a 
/// way to associate TraceCategoryId values with human readable names.
///
class TraceCategory {
public:
    /// Computes an id for the given a string literal \p str.
    template <int N>
    static constexpr TraceCategoryId CreateTraceCategoryId(
        const char (&str)[N]) {
        return TraceStringHash::Hash(str);
    }

    /// Default category if none are explicitly specified when creating a 
    /// TraceEvent.
    enum : TraceCategoryId { Default = 0 };

    /// Associates the \p id with \p name. These associates are not necessarily 
    /// unique.
    TRACE_API void RegisterCategory(TraceCategoryId id, const std::string& name);

    /// Returns all names associated with the \p id.
    TRACE_API std::vector<std::string> GetCategories(TraceCategoryId id) const;

    /// Singleton accessor.
    TRACE_API static TraceCategory& GetInstance();

private:
    friend class TfSingleton<TraceCategory>;

    TraceCategory();

    // Mapping of ids to names.
    std::multimap<TraceCategoryId, std::string> _idToNames;
};

TRACE_API_TEMPLATE_CLASS(TfSingleton<TraceCategory>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TRACE_CATEGORY_H
