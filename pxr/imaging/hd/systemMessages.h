//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_SYSTEM_MESSAGES_H
#define PXR_IMAGING_HD_SYSTEM_MESSAGES_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


#define HD_SYSTEM_MESSAGE_TOKENS                                               \
    /* Indicates that asynchronous processing is allowed and to expect to
     * receive "asyncPoll" messages to follow. This message provides no
     * arguments.
     */                                                                        \
   (asyncAllow)                                                                \
   /* Following a "asyncAllow" message, this will be called periodically on the
     * application main (or rendering) thread to give scene indices an
     * opportunity to send notices for completed asynchronous or incremental
     * work.
     */                                                                        \
   (asyncPoll)                                                                 \

TF_DECLARE_PUBLIC_TOKENS(HdSystemMessageTokens, HD_API,
    HD_SYSTEM_MESSAGE_TOKENS);


PXR_NAMESPACE_CLOSE_SCOPE

#endif