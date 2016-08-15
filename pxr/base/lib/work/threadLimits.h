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
#ifndef WORK_THREAD_LIMITS_H
#define WORK_THREAD_LIMITS_H

/// \file work/threadLimits.h

/// Return the current concurrency limit, always >= 1.
///
/// The initial value is determined by the PXR_WORK_THREAD_LIMIT env setting,
/// which defaults to WorkGetMaximumConcurrencyLimit().
///
/// Note that this can return a value larger than
/// WorkGetMaximumConcurrencyLimit() if WorkSetConcurrencyLimit() was called
/// with such a value.
///
unsigned WorkGetConcurrencyLimit();

/// Return the number of physical execution cores available to the program.
/// This is either the number of physical cores on the machine or the number of
/// cores specified by the process's affinity mask, whichever is smaller.
///
unsigned WorkGetMaximumConcurrencyLimit();

/// Set the concurrencty limit to max(1, \p n).  
///
/// Note, calling ths function with n > WorkGetMaximumConcurrencyLimit() may
/// overtax the machine.
///
/// In general, very few places should call this function.  Call it in places
/// where the number of allowed threads is dictated, for example, by a hosting
/// environment.  Lower-level library code should never call this function.
///
void WorkSetConcurrencyLimit(unsigned n);

/// Sanitize \p n as described below and set the concurrency limit accordingly.
/// This function is useful for interpreting command line arguments.
///
/// If \p n is between 1 and WorkGetMaximumConcurrencyLimit() 
/// (inclusive) then call WorkSetConcurrencyLimit(n).
///
/// If \p n is 0 or larger than the number of physical cores, call
/// WorkSetMaximumConcurrencyLimit().
///
/// If \p n is negative, then set the concurrency limit to all but abs(\p n)
/// cores are used.  For examlpe, if \p n is -2, then use all but two cores.  If
/// abs(\p n) is greater than the number of physical cores, then call
/// WorkSetConcurrencyLimit(1), effectively disabling concurrency.
///
void WorkSetConcurrencyLimitArgument(int n);

/// Set the concurrency limit to be the maximum recommended for the hardware
/// on which it's running.  Equivalent to:
/// \code
/// WorkSetConcurrencyLimit(WorkGetMaximumConcurrencyLimit()).
/// \endcode
///
void WorkSetMaximumConcurrencyLimit();

#endif
