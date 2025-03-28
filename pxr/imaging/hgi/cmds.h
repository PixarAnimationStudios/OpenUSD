//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_CMDS_H
#define PXR_IMAGING_HGI_CMDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;

using HgiCmdsUniquePtr = std::unique_ptr<class HgiCmds>;


/// \class HgiCmds
///
/// Graphics commands are recorded in 'cmds' objects which are later submitted
/// to hgi. HgiCmds is the base class for other cmds objects.
///
class HgiCmds
{
public:
    HGI_API
    virtual ~HgiCmds();

    /// Returns true if the HgiCmds object has been submitted to GPU.
    HGI_API
    bool IsSubmitted() const;

protected:
    friend class Hgi;

    HGI_API
    HgiCmds();

    // Submit can be called inside of Hgi::SubmitCmds to commit the
    // command buffer to the GPU. Returns true if work was committed.
    // The default implementation returns false.
    HGI_API
    virtual bool _Submit(Hgi* hgi, HgiSubmitWaitType wait);

    // Flags the HgiCmds object as 'submitted' to GPU.
    HGI_API
    void _SetSubmitted();

private:
    HgiCmds & operator=(const HgiCmds&) = delete;
    HgiCmds(const HgiCmds&) = delete;

    bool _submitted;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
