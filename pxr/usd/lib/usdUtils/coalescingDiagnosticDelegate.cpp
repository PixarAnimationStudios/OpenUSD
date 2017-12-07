//
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

#include "pxr/pxr.h"

#include "pxr/usd/usdUtils/coalescingDiagnosticDelegate.h"
#include "pxr/base/tf/stackTrace.h"
#include "pxr/base/arch/debugger.h"

#include <memory>
#include <tuple>
#include <unordered_map>

#include <boost/functional/hash.hpp>


PXR_NAMESPACE_OPEN_SCOPE

// Hash implementation which allows us to coalesce warnings
// and statuses based on their file name, line number and 
// function name.
namespace {
    using _CoalescedItem = UsdUtilsCoalescingDiagnosticDelegateSharedItem;

    struct _CoalescedItemHash {
        std::size_t operator()(const _CoalescedItem& i) const {
            std::size_t hashVal = 0;
            boost::hash_combine(hashVal, i.sourceLineNumber);
            boost::hash_combine(hashVal, i.sourceFunction);
            boost::hash_combine(hashVal, i.sourceFileName);
            return hashVal;
        }
    };

    struct _CoalescedItemEqualTo {
        bool operator()(const _CoalescedItem& i1, 
                        const _CoalescedItem& i2) const {
            return i1.sourceLineNumber == i2.sourceLineNumber
                   && i1.sourceFunction == i2.sourceFunction
                   && i1.sourceFileName == i2.sourceFileName;    
        }
    };
}

UsdUtilsCoalescingDiagnosticDelegate::UsdUtilsCoalescingDiagnosticDelegate() 
{
   TfDiagnosticMgr::GetInstance().AddDelegate(this);
}

UsdUtilsCoalescingDiagnosticDelegate::~UsdUtilsCoalescingDiagnosticDelegate()
{
    TfDiagnosticMgr::GetInstance().RemoveDelegate(this);
}

void 
UsdUtilsCoalescingDiagnosticDelegate::IssueError(const TfError &err)
{
    // UsdUtilsCoalescingDiagnosticDelegate does not do anything with errors.
    // Consider using a TfErrorMark for these cases. 
}

void 
UsdUtilsCoalescingDiagnosticDelegate::IssueFatalError(const TfCallContext &ctx, 
                                                      const std::string &msg)
{
    TfLogCrash("FATAL ERROR", msg, std::string() /*additionalInfo*/,
               ctx, true /*logToDB*/);
    ArchAbort(/*logging=*/ false);    
}

void 
UsdUtilsCoalescingDiagnosticDelegate::IssueStatus(const TfStatus &status)
{
    _diagnostics.push(new TfDiagnosticBase(status));
}

void 
UsdUtilsCoalescingDiagnosticDelegate::IssueWarning(const TfWarning &warning)
{
    _diagnostics.push(new TfDiagnosticBase(warning));
}

UsdUtilsCoalescingDiagnosticDelegateVector
UsdUtilsCoalescingDiagnosticDelegate::TakeCoalescedDiagnostics()
{
    std::unordered_map<UsdUtilsCoalescingDiagnosticDelegateSharedItem, size_t,
         _CoalescedItemHash, _CoalescedItemEqualTo> existence;

    UsdUtilsCoalescingDiagnosticDelegateVector result;

    // Our map will correspond coalesced items with an index in our resulting
    // vector, so the first time we see an item of a certain grouping we mark
    // its position to maintain some sort of relative ordering.
    size_t vectorIndex = 0;

    TfDiagnosticBase* handle;
    while (!_diagnostics.empty()) {
        if (_diagnostics.try_pop(handle)) {
            UsdUtilsCoalescingDiagnosticDelegateSharedItem sharedItem {
                handle->GetSourceLineNumber(),
                handle->GetSourceFunction(),
                handle->GetSourceFileName(), 
            };

            UsdUtilsCoalescingDiagnosticDelegateUnsharedItem unsharedItem {
                handle->GetContext(),
                handle->GetCommentary()
            };

            auto lookup = existence.find(sharedItem);
            if (lookup == existence.end()) {
                existence.insert(std::make_pair(sharedItem, vectorIndex));
                UsdUtilsCoalescingDiagnosticDelegateItem vItem {
                    sharedItem,
                    { unsharedItem } 
                };

                result.push_back(vItem);
                vectorIndex += 1;
            } else {
                result[lookup->second].unsharedItems.push_back(unsharedItem);
            }

            delete handle;
        }
    }

    return result;
}

std::vector<std::unique_ptr<TfDiagnosticBase>>
UsdUtilsCoalescingDiagnosticDelegate::TakeUncoalescedDiagnostics()
{
    std::vector<std::unique_ptr<TfDiagnosticBase>> items;

    TfDiagnosticBase* handle;
    while (!_diagnostics.empty()) {
        if (_diagnostics.try_pop(handle)) {
            items.push_back(std::unique_ptr<TfDiagnosticBase>(
                               new TfDiagnosticBase(*handle)));
            delete handle;
        }
    }

    return items;
}

void
UsdUtilsCoalescingDiagnosticDelegate::DumpCoalescedDiagnostics(std::ostream& o)
{
    for (auto const& item : TakeCoalescedDiagnostics()) {
        o << item.unsharedItems.size() << " "; 
        o << "Diagnostic Notification(s) in ";
        o << item.sharedItem.sourceFunction;
        o << " at line "            << item.sharedItem.sourceLineNumber;
        o << " of "                 << item.sharedItem.sourceFileName;
        o << "\n";
    }
}

void
UsdUtilsCoalescingDiagnosticDelegate::DumpUncoalescedDiagnostics(std::ostream& o)
{
    for (auto const& item : TakeUncoalescedDiagnostics()) {
        o << "Diagnostic Notification in ";
        o << item->GetSourceFunction();
        o << " at line "            << item->GetSourceLineNumber();
        o << " of "                 << item->GetSourceFileName();
        o << ":\n   "               << item->GetCommentary(); 
        o << "\n";
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
