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
    TF_WRAP_NB( AnyWeakPtr );
    TF_WRAP_NB( CallContext );
    TF_WRAP_NB( Debug );
    TF_WRAP_NB( Enum );
    // Diagnostic depends on Enum so must come after it.
    TF_WRAP_NB( Diagnostic );
    TF_WRAP_NB( DiagnosticBase );
    TF_WRAP_NB( DiagnosticTrap );
    TF_WRAP_NB( EnvSetting );
    TF_WRAP_NB( Error );
    TF_WRAP_NB( Exception );
    TF_WRAP_NB( FileUtils );
    TF_WRAP_NB( Function );
    TF_WRAP_NB( MallocTag );
    TF_WRAP_NB( Notice );
    TF_WRAP_NB( PathUtils );
    TF_WRAP_NB( PyContainerConversions );
    TF_WRAP_NB( PyModuleNotice );
    TF_WRAP_NB( PyObjWrapper );
    TF_WRAP_NB( PyOptional );
    TF_WRAP_NB( RefPtrTracker );
    TF_WRAP_NB( ScopeDescription );
    TF_WRAP_NB( ScriptModuleLoader );
    TF_WRAP_NB( Singleton );
    TF_WRAP_NB( Status );
    TF_WRAP_NB( StackTrace );
    TF_WRAP_NB( Stopwatch );
    TF_WRAP_NB( StringUtils );
    TF_WRAP_NB( Token );
    TF_WRAP_NB( Type );
    TF_WRAP_NB( Tf_TestPyAnnotatedBoolResult );
    TF_WRAP_NB( Tf_TestPyContainerConversions );
    TF_WRAP_NB( Tf_TestPyStaticTokens );
    TF_WRAP_NB( Tf_TestTfPython );
    TF_WRAP_NB( Tf_TestTfPyOptional );
    TF_WRAP_NB( Warning );
}
