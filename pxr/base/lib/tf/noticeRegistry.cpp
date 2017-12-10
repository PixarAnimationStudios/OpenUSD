//
// Copyright 2016 Pixar
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
#include "pxr/base/tf/noticeRegistry.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/arch/demangle.h"

#include <typeinfo>

using std::string;
using std::vector;
using std::type_info;

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(Tf_NoticeRegistry);

Tf_NoticeRegistry::Tf_NoticeRegistry()
    : _userCount(0),
      _doProbing(false),
      _globalBlockCount(0)
{
    /*
     * lib/tf's diagnostic-reporting mechanisms are based on sending
     * a few TfNotice types.
     *
     * However, if the type system itself is screwed up, we can find
     * ourselves sending notices even before the type system has had a
     * change to boot itself!  To avoid an infinite loop, we need to
     * define the basic error types of tf; because if these are NOT
     * defined, the notice system helpfully sends you an error notice,
     * which is itself not defined, which results in an error
     * notice... etc.
     */

    TfSingleton<Tf_NoticeRegistry>::SetInstanceConstructed(*this);
}

/*
 * This method is called when a notice delivery needs to fall back
 * to TfSafeDynamic_cast.
 */
void
Tf_NoticeRegistry::_VerifyFailedCast(const type_info& toType,
                                     const TfNotice& notice,
                                     const TfNotice* castNotice)
{
    string typeName = ArchGetDemangled(typeid(notice));

    if (castNotice) {
        {
            _Lock lock(_warnMutex);
            if (_warnedBadCastTypes.count(typeName))
                return;
            else
                _warnedBadCastTypes.insert(typeName);
        }
        /*
         * TfSafeDynamic_cast worked, but the class needs a virtual function
         * to avoid this in the future.  Warn so the author of the notice class
         * can fix.
         */
        TF_WARN("Special handling of notice type '%s' invoked.\n"
                "Most likely, this class is missing a non-inlined "
                "virtual destructor.\n"
                "Please request that someone modify class '%s' "
                "accordingly.", typeName.c_str(), typeName.c_str());
        return;
    }

    /*
     * Death.
     */

    TF_FATAL_ERROR("All attempts to cast notice of type '%s' to type "
                   "'%s' failed.  One possibility is that '%s' has no "
                   "non-inlined virtual functions and this system's C++ "
                   "ABI is non-standard.  Verify that class '%s'"
                   "has at least one non-inline virtual function.\n",
                   typeName.c_str(), ArchGetDemangled(toType).c_str(),
                   typeName.c_str(), typeName.c_str());
}

void
Tf_NoticeRegistry::_InsertProbe(const TfNotice::WeakProbePtr &probe)
{
    _Lock lock(_probeMutex);
    if (probe)
        _probes.insert(probe);
    _doProbing = !_probes.empty();
}


void
Tf_NoticeRegistry::_RemoveProbe(const TfNotice::WeakProbePtr &probe)
{
    _Lock lock(_probeMutex);
    _probes.erase(probe);
    _doProbing = !_probes.empty();
}

void
Tf_NoticeRegistry::
_BeginSend(const TfNotice &notice,
            const TfWeakBase *sender,
            const std::type_info &senderType,
            const std::vector<TfNotice::WeakProbePtr> &probes)
{
    TF_FOR_ALL(i, probes)
        if (*i)
            (*i)->BeginSend(notice, sender, senderType);
}

void
Tf_NoticeRegistry::_EndSend(const std::vector<TfNotice::WeakProbePtr> &probes)
{
    TF_FOR_ALL(i, probes)
        if (*i)
            (*i)->EndSend();
}

void
Tf_NoticeRegistry::
_BeginDelivery(const TfNotice &notice,
               const TfWeakBase *sender,
               const std::type_info &senderType,
               const TfWeakBase *listener,
               const std::type_info &listenerType,
               const std::vector<TfNotice::WeakProbePtr> &probes)
{
    TF_FOR_ALL(i, probes)
        if (*i)
            (*i)->BeginDelivery(notice, sender,
                                senderType, listener, listenerType);
}

void
Tf_NoticeRegistry::
_EndDelivery(const std::vector<TfNotice::WeakProbePtr> &probes)
{
    TF_FOR_ALL(i, probes)
        if (*i)
            (*i)->EndDelivery();
}

TfNotice::Key
Tf_NoticeRegistry::_Register(TfNotice::_DelivererBase* deliverer)
{
    TfAutoMallocTag2 tag("Tf", "Tf_NoticeRegistry::_Register");

    TfType noticeType = deliverer->GetNoticeType();
    
    if (noticeType.IsUnknown()) {
        TF_FATAL_ERROR("notice type is undefined in the TfType system");
    }

    _IncrementUserCount(1);

    _DelivererContainer* container =
        _GetOrCreateDelivererContainer(noticeType);
    _Prepend(container, deliverer->GetSenderWeakBase(), deliverer);

    _IncrementUserCount(-1);

    return TfNotice::Key(TfCreateWeakPtr(deliverer));
}

void
Tf_NoticeRegistry::_Revoke(TfNotice::Key& key)
{
    _Lock lock(_userCountMutex);

    if (_userCount == 0) {
        // If no other execution context is traversing the registry, we
        // can remove the deliverer immediately.
        _FreeDeliverer(key._deliverer);
    } else {
        // Otherwise deactivate it.
        key._deliverer->_Deactivate();
    }
}

size_t
Tf_NoticeRegistry::_Send(const TfNotice &n, const TfType & noticeType,
                         const TfWeakBase *s, const void *senderUniqueId,
                         const std::type_info &senderType)
{
    // Check the global block count to avoid the overhead of looking
    // up the thread-specific data in the 99.9% case where a block 
    // is not present.
    if (_globalBlockCount > 0) {
        if (_perThreadBlockCount.local() > 0) {
            return 0;
        }
    }

    _IncrementUserCount(1);

    size_t nSent = 0;

    vector< TfNotice::WeakProbePtr > probeList;
    bool doProbing = _doProbing;
    if (doProbing) {
        // Copy off a list of the probes.
        _Lock lock(_probeMutex);
        probeList.reserve(_probes.size());
        TF_FOR_ALL(i, _probes) {
            if (*i) {
                probeList.push_back(*i);
            }
        }
        doProbing = !probeList.empty();
        if (doProbing) {
            _BeginSend(n, s, senderType, probeList);
        }
    }

    // Deliver notice, walking up the chain of base types.
    TfType t = noticeType;
    do {
        if (_DelivererContainer* container = _GetDelivererContainer(t)) {
            if (s) {
                // Do per-sender listeners
                nSent += _Deliver(n, noticeType, s, senderUniqueId,
                                  senderType, probeList,
                                  _GetHeadForSender(container, s));
            }
            // Do "global" listeners
            nSent += _Deliver(n, noticeType, s, senderUniqueId,
                              senderType, probeList, _GetHead(container));
        }

        // Chain up base types to find listeners interested in them
        const vector<TfType> & parents = t.GetBaseTypes();
        if (parents.size() != 1)
            _BadTypeFatalMsg(t, typeid(n));
        t = parents[0];
    } while (t != TfType::GetRoot());

    if (doProbing) {
        _EndSend(probeList);
    }

    // Decrement _userCount, and if there are no other execution contexts
    // using the notice registry, clean out expired deliverers.
    {
        _Lock lock(_userCountMutex);

        if (_userCount == 1 && !_deadEntries.empty()) {
            for (size_t i=0, n=_deadEntries.size(); i!=n; ++i) {
                _FreeDeliverer(_deadEntries[i]);
            }
            _deadEntries.clear();
        }
                
        --_userCount;
    }

    return nSent;
}

int
Tf_NoticeRegistry::_Deliver(const TfNotice &n, const TfType &type,
                             const TfWeakBase *s,
                             const void *senderUniqueId,
                             const std::type_info &senderType,
                             const std::vector<TfNotice::WeakProbePtr> &probes,
                             const _DelivererListEntry & entry)
{ 
    _DelivererList *dlist = entry.first;
    if (!dlist)
        return 0;

    int nSent = 0;
    _DelivererList::iterator i = entry.second;
    while (i != dlist->end()) {
        _DelivererList::value_type deliverer = *i;
        if (deliverer->_IsActive() && deliverer->
            _SendToListener(n, type, s, senderUniqueId, senderType, probes)) {
            ++nSent;
        } else {
            _Lock lock(_userCountMutex);
            if (!deliverer->_IsMarkedForRemoval()) {
                deliverer->_Deactivate();
                deliverer->_MarkForRemoval();
                _deadEntries.push_back(TfCreateWeakPtr(deliverer));
            }
        }
        ++i;
    }
    return nSent;
}

void
Tf_NoticeRegistry::_FreeDeliverer(const TfNotice::_DelivererWeakPtr & d)
{
    if (d) {
        _DelivererList *list = d->_list;
        _DelivererList::iterator iter = d->_listIter;
        delete get_pointer(d);
        list->erase(iter);
    }
}

void
Tf_NoticeRegistry::_BadTypeFatalMsg(const TfType& t,
                                    const std::type_info& ti)
{
    const vector<TfType> &baseTypes = t.GetBaseTypes();
    string msg;
            
    if (t.IsUnknown()) {
        msg = TfStringPrintf("Class %s (derived from TfNotice) is "
                             "undefined in the TfType system",
                             ArchGetDemangled(ti).c_str());
    }
    else if (!baseTypes.empty()) {
        msg = TfStringPrintf("TfNotice type '%s' has multiple base types;\n"
                             "it must have a unique parent in the TfType system",
                             t.GetTypeName().c_str());                       
    }
    else {
        msg = TfStringPrintf("TfNotice type '%s' has NO base types;\n"
                             "this should be impossible.",
                             t.GetTypeName().c_str());
    }

    TF_FATAL_ERROR(msg);
}

void 
Tf_NoticeRegistry::_IncrementBlockCount()
{
    ++_globalBlockCount;
    ++_perThreadBlockCount.local();
}

void 
Tf_NoticeRegistry::_DecrementBlockCount()
{
    --_globalBlockCount;
    --_perThreadBlockCount.local();
}

PXR_NAMESPACE_CLOSE_SCOPE
