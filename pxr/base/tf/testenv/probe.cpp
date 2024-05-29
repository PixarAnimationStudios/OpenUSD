//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/arch/systemInfo.h"
#include <string>
#include <vector>
#include <stdio.h>
#include <iostream>
#include <sstream>

using namespace std;
PXR_NAMESPACE_USING_DIRECTIVE

int beginSendCount = 0;
int endSendCount = 0;
int beginDeliveryCount = 0;
int endDeliveryCount = 0;
int processedNoticeCount = 0;

class _NoticeProbe : public TfNotice::Probe
{
  public:
    _NoticeProbe() {};
    virtual ~_NoticeProbe() {};    

    virtual void BeginSend( const TfNotice &notice,
                            const TfWeakBase *sender,
                            const std::type_info &senderType )
    {
        beginSendCount++;
    }
    virtual void EndSend()
    {
        endSendCount++;
    }

    virtual void BeginDelivery( const TfNotice &notice,
                                const TfWeakBase *sender,
                                const std::type_info &senderType,
                                const TfWeakBase *listener,
                                const std::type_info &listenerType )
    {
        beginDeliveryCount++;
    }
    virtual void EndDelivery()
    {
        endDeliveryCount++;
    }
};

class ProbeNotice : public TfNotice {
public:
    ProbeNotice(const string& what)
        : _what(what) {
    }
    ~ProbeNotice();

    const string& GetWhat() const {
        return _what;
    }
    
private:
  const string _what;
};

class ProbeListener : public TfWeakBase {
public:
    explicit ProbeListener()
    {
    }

    //! Called when a notice of any type is sent
    void ProcessNotice(const TfNotice&) {
        processedNoticeCount++;
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<ProbeNotice, TfType::Bases<TfNotice> >();
}

ProbeNotice::~ProbeNotice() { }

static bool
Test_TfProbe()
{
    _NoticeProbe _probe;
    ProbeListener *l1 = new ProbeListener();
    TfWeakPtr<ProbeListener> wl1(l1);

    TfNotice::Register(wl1, &ProbeListener::ProcessNotice);

    TfNotice::InsertProbe(TfCreateWeakPtr( &_probe ));

    ProbeNotice("first").Send();
    TfNotice::RemoveProbe(TfCreateWeakPtr( &_probe ));

    ProbeNotice("second").Send();

    TF_AXIOM(beginSendCount == 1);
    TF_AXIOM(endSendCount == 1);
    TF_AXIOM(beginDeliveryCount == 1);
    TF_AXIOM(endDeliveryCount == 1);
    TF_AXIOM(processedNoticeCount == 2);

    return true;
}

TF_ADD_REGTEST(TfProbe);
