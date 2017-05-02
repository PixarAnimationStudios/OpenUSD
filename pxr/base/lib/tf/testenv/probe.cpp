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
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/notice.h"
#include "pxr/base/tf/noticeRegistry.h"
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

    Tf_NoticeRegistry::_GetInstance()._InsertProbe(TfCreateWeakPtr( &_probe ));

    ProbeNotice("first").Send();
    Tf_NoticeRegistry::_GetInstance()._RemoveProbe(TfCreateWeakPtr( &_probe ));

    ProbeNotice("second").Send();

    TF_AXIOM(beginSendCount == 1);
    TF_AXIOM(endSendCount == 1);
    TF_AXIOM(beginDeliveryCount == 1);
    TF_AXIOM(endDeliveryCount == 1);
    TF_AXIOM(processedNoticeCount == 2);

    return true;
}

TF_ADD_REGTEST(TfProbe);
