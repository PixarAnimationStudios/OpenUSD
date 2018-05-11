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
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE {
    TF_WRAP( AnyWeakPtr );
    TF_WRAP( CallContext );
    TF_WRAP( Debug );
    TF_WRAP( Enum );
    // Diagnostic depends on Enum so must come after it.
    TF_WRAP( Diagnostic );
    TF_WRAP( DiagnosticBase );
    TF_WRAP( EnvSetting );
    TF_WRAP( Error );
    TF_WRAP( FileUtils );
    TF_WRAP( Function );
    TF_WRAP( MallocTag );
    TF_WRAP( Notice );
    TF_WRAP( PathUtils );
    TF_WRAP( PyContainerConversions );
    TF_WRAP( PyModuleNotice );
    TF_WRAP( PyObjWrapper );
    TF_WRAP( PyOptional );
    TF_WRAP( RefPtrTracker );
    TF_WRAP( ScopeDescription );
    TF_WRAP( ScriptModuleLoader );
    TF_WRAP( Singleton );
    TF_WRAP( Status );
    TF_WRAP( StackTrace );
    TF_WRAP( Stopwatch );
    TF_WRAP( StringUtils );
    TF_WRAP( TemplateString );
    TF_WRAP( Token );
    TF_WRAP( Type );
    TF_WRAP( Tf_TestPyAnnotatedBoolResult );
    TF_WRAP( Tf_TestPyContainerConversions );
    TF_WRAP( Tf_TestPyStaticTokens );
    TF_WRAP( Tf_TestTfPython );
    TF_WRAP( Tf_TestTfPyOptional );
    TF_WRAP( Warning );
}
