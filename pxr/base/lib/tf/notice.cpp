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
#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/noticeRegistry.h"
#include "pxr/base/tf/type.h"

using std::type_info;
using std::vector;



// Note:  We do not register a TfType for TfNotice here.
// Instead, we register it in Type.cpp.  See Tf_TypeRegistry's constructor.

TfNotice::~TfNotice()
{
}


TfNotice::_DelivererBase::~_DelivererBase()
{
}

void
TfNotice::_DelivererBase::
_BeginDelivery(const TfNotice &notice,
               const TfWeakBase *sender,
               const type_info &senderType,
               const TfWeakBase *listener,
               const type_info &listenerType,
               const vector<TfNotice::WeakProbePtr> &probes)
{
    Tf_NoticeRegistry::_GetInstance().
        _BeginDelivery(notice, sender, senderType,
                        listener, listenerType, probes);
}

void
TfNotice::_DelivererBase::
_EndDelivery(const vector<TfNotice::WeakProbePtr> &probes)
{
    Tf_NoticeRegistry::_GetInstance()._EndDelivery(probes);
}

TfNotice::Probe::~Probe()
{
}


void
TfNotice::InsertProbe(const WeakProbePtr &probe)
{
    Tf_NoticeRegistry::_GetInstance()._InsertProbe(probe);
}

void
TfNotice::RemoveProbe(const WeakProbePtr &probe)
{
    Tf_NoticeRegistry::_GetInstance()._RemoveProbe(probe);
}

TfNotice::Key
TfNotice::_Register(_DelivererBase *deliverer)
{
    return Tf_NoticeRegistry::_GetInstance()._Register(deliverer);
}

size_t 
TfNotice::_Send(const TfWeakBase *s,
                const void *senderUniqueId,
                const type_info &senderType) const
{
    // Look up the notice type using the static type_info.
    // This is faster than TfType::Find().
    TfType noticeType = TfType::Find(typeid(*this));

    return Tf_NoticeRegistry::_GetInstance().
        _Send(*this, noticeType, s, senderUniqueId, senderType);
}

size_t 
TfNotice::_SendWithType(const TfType &noticeType,
                        const TfWeakBase *s,
                        const void *senderUniqueId,
                        const type_info &senderType) const
{
    return Tf_NoticeRegistry::_GetInstance().
        _Send(*this, noticeType, s, senderUniqueId, senderType);
}

size_t
TfNotice::Send() const
{
    return _Send(0, 0, typeid(void));
}

size_t 
TfNotice::SendWithWeakBase(const TfWeakBase *senderWeakBase,
                           const void *senderUniqueId,
                           const std::type_info &senderType) const
{
    return _Send(senderWeakBase, senderUniqueId,
                 senderWeakBase ? senderType : typeid(void));
}

bool
TfNotice::Revoke(Key& key)
{
    if (!key) {
        return false;
    }
    
    Tf_NoticeRegistry::_GetInstance()._Revoke(key);

    return true;
}

void
TfNotice::Revoke(Keys* keys)
{
    TF_FOR_ALL(i, *keys) {
        Revoke(*i);
    }
    keys->clear();
}

void
TfNotice::_VerifyFailedCast(const type_info& toType,
                            const TfNotice& notice, const TfNotice* castNotice)
{
    Tf_NoticeRegistry::_GetInstance()
        ._VerifyFailedCast(toType, notice, castNotice);
}

TfNotice::Block::Block()
{
    Tf_NoticeRegistry::_GetInstance()._IncrementBlockCount();
}

TfNotice::Block::~Block()
{
    Tf_NoticeRegistry::_GetInstance()._DecrementBlockCount();
}

