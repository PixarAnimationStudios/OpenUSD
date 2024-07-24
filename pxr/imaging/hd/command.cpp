//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/command.h"

PXR_NAMESPACE_OPEN_SCOPE

// We cannot use a defaulted constructor for HdCommandDescriptor because
// of the const member variables. On some compilers and STL implementations
// the commandArgs member (which is a std::vector) does not have a
// user-provided c'tor, which C++ requires in order to generate a default
// c'tor for HdCommandDescriptor. So, we have to create an explicitly
// empty c'tor to avoid this problem.
//
// HdCommandArgDescriptor could use a defaulted constructor but since it
// also uses const member variables, using an explicit empty c'tor guards
// against the above issue should any members be added or changed.

HdCommandArgDescriptor::HdCommandArgDescriptor()
{
}

HdCommandDescriptor::HdCommandDescriptor()
{
}

PXR_NAMESPACE_CLOSE_SCOPE
