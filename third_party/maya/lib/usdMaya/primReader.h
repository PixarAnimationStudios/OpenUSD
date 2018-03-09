//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYA_MAYAPRIMREADER_H
#define PXRUSDMAYA_MAYAPRIMREADER_H

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/primReaderArgs.h"
#include "usdMaya/primReaderContext.h"

PXR_NAMESPACE_OPEN_SCOPE

class PxrUsdMayaPrimReader
{
public:
    PXRUSDMAYA_API
    PxrUsdMayaPrimReader(
            const PxrUsdMayaPrimReaderArgs&,
            PxrUsdMayaPrimReaderContext*);
    virtual ~PxrUsdMayaPrimReader() {};

    /// Reads the USD prim given by the prim reader args into a Maya shape,
    /// modifying the prim reader context as a result.
    /// Returns true if successful.
    virtual bool Read() = 0;

    /// Whether this prim reader specifies a PostReadSubtree step.
    virtual bool HasPostReadSubtree() const;

    /// An additional import step that runs after all descendants of this prim
    /// have been processed.
    /// For example, if we have prims /A, /A/B, and /C, then the import steps
    /// are run in the order:
    /// (1) Read A (2) Read B (3) PostReadSubtree B (4) PostReadSubtree A,
    /// (5) Read C (6) PostReadSubtree C
    virtual void PostReadSubtree();

protected:
    /// Input arguments. Read data about the input USD prim from here.
    const PxrUsdMayaPrimReaderArgs& _GetArgs();
    /// Output context. Write data here to inform the read job about any Maya
    /// nodes added as well as further traversal behavior.
    PxrUsdMayaPrimReaderContext* _GetContext();

private:
    const PxrUsdMayaPrimReaderArgs& _args;
    PxrUsdMayaPrimReaderContext* _context;
};

typedef std::shared_ptr<PxrUsdMayaPrimReader> PxrUsdMayaPrimReaderPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_MAYAPRIMREADER_H
