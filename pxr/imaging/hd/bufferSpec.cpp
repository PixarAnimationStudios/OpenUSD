//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/base/tf/hash.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


/*static*/
bool
HdBufferSpec::IsSubset(HdBufferSpecVector const &subset,
                       HdBufferSpecVector const &superset)
{
    HD_TRACE_FUNCTION();

    for(HdBufferSpec const& spec : subset) {
        if (std::find(superset.begin(), superset.end(), spec) == superset.end())
            return false;
    }
    return true;
}

/*static*/
HdBufferSpecVector
HdBufferSpec::ComputeUnion(HdBufferSpecVector const &specs1,
                           HdBufferSpecVector const &specs2)
{
    // This implementation assumes small inputs.

    HD_TRACE_FUNCTION();

    HdBufferSpecVector r;

    for (HdBufferSpec const& s : specs1) {
        if (std::find(r.begin(), r.end(), s) != r.end()) {
            continue;
        }
        r.push_back(s);
    }
    for (HdBufferSpec const& s : specs2) {
        if (std::find(r.begin(), r.end(), s) != r.end()) {
            continue;
        }
        r.push_back(s);
    }

    return r;
}

/*static*/
HdBufferSpecVector
HdBufferSpec::ComputeDifference(HdBufferSpecVector const &specs1,
                                HdBufferSpecVector const &specs2)
{
    // This implementation assumes small inputs.

    HD_TRACE_FUNCTION();

    HdBufferSpecVector r;

    for(HdBufferSpec const& s : specs1) {
        if (std::find(specs2.begin(), specs2.end(), s) != specs2.end()) {
            continue;
        }
        if (std::find(r.begin(), r.end(), s) != r.end()) {
            continue;
        }
        r.push_back(s);
    }

    return r;
}

size_t
HdBufferSpec::Hash() const
{
    return TfHash()(*this);
}

void
HdBufferSpec::Dump(HdBufferSpecVector const &specs)
{
    std::cout << "BufferSpecVector\n";
    for (int i = 0; i < (int)specs.size(); ++i) {
        std::cout << i << " : "
                  << specs[i].name << ", "
                  << TfEnum::GetName(specs[i].tupleType.type) << " ("
                  << specs[i].tupleType.type << "), "
                  << specs[i].tupleType.count << "\n";
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

