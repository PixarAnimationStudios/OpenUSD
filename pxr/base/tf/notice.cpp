//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/noticeRegistry.h"
#include "pxr/base/tf/type.h"

using std::type_info;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

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

PXR_NAMESPACE_CLOSE_SCOPE
