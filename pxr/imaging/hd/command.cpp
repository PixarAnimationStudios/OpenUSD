//
// Copyright 2021 Pixar
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
