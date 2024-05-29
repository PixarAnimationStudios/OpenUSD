//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    TF_WRAP( Exception );
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
