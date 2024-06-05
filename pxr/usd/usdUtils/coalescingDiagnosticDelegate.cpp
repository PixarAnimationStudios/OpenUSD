//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/usd/usdUtils/coalescingDiagnosticDelegate.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/stackTrace.h"
#include "pxr/base/arch/debugger.h"

#include <memory>
#include <tuple>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

// Hash implementation which allows us to coalesce warnings
// and statuses based on their file name, line number and 
// function name.
namespace {
    using _CoalescedItem = UsdUtilsCoalescingDiagnosticDelegateSharedItem;

    struct _CoalescedItemHash {
        std::size_t operator()(const _CoalescedItem& i) const {
            return TfHash::Combine(
                i.sourceLineNumber, i.sourceFunction, i.sourceFileName);
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

    // Call TakeUncoalescedDiagnostics and then drop the result immediately
    // to clean up any diagnostics that remain in the delegate. 
    //
    // This must be done after RemoveDelegate to ensure we don't enqueue
    // more diagnostics after we've tried to clean up.
    TakeUncoalescedDiagnostics();
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
    _diagnostics.push(new TfStatus(status));
}

void 
UsdUtilsCoalescingDiagnosticDelegate::IssueWarning(const TfWarning &warning)
{
    _diagnostics.push(new TfWarning(warning));
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

    TfDiagnosticBase* p = nullptr;
    while (!_diagnostics.empty()) {
        if (_diagnostics.try_pop(p)) {
            std::unique_ptr<TfDiagnosticBase> handle(p);
            
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
        }
    }

    return result;
}

std::vector<std::unique_ptr<TfDiagnosticBase>>
UsdUtilsCoalescingDiagnosticDelegate::TakeUncoalescedDiagnostics()
{
    std::vector<std::unique_ptr<TfDiagnosticBase>> items;

    TfDiagnosticBase* p = nullptr;
    while (!_diagnostics.empty()) {
        if (_diagnostics.try_pop(p)) {
            items.push_back(std::unique_ptr<TfDiagnosticBase>(p));
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
