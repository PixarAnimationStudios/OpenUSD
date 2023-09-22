//
// Copyright 2020 Pixar
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

#include "TestArURIResolver_plugin.h"

#include "pxr/usd/ar/defaultResolverContext.h"
#include "pxr/usd/ar/defineResolverContext.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/usd/ar/resolverContextBinder.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_USING_DIRECTIVE;

static void
SetupPlugins()
{
    // Register TestArURIResolver plugin. We assume the build system will
    // install it to the ArPlugins subdirectory in the same location as
    // this test.
    const std::string uriResolverPluginPath =
        TfStringCatPaths(
            TfGetPathName(ArchGetExecutablePath()),
            "ArPlugins/lib/TestArURIResolver*/Resources/") + "/";

    PlugPluginPtrVector plugins =
        PlugRegistry::GetInstance().RegisterPlugins(uriResolverPluginPath);
    
    TF_AXIOM(plugins.size() == 1);
    TF_AXIOM(plugins[0]->GetName() == "TestArURIResolver");

    // Set the preferred resolver to a resolver marked as primary but also specifies uriSchemes
    // before running any test cases.
    ArSetPreferredResolver("_TestOtherPrimaryResolver");
}

static void
TestResolveWithContext()
{
    ArResolver& resolver = ArGetResolver();

    // Verify that the context object is getting bound in the _TestURIResolver.
    // The test resolver simply appends the string in the context object
    // to the end of the given path when resolving.
    ArResolverContext ctx(_TestURIResolverContext("context"));
    ArResolverContextBinder binder(ctx);
    TF_AXIOM(resolver.Resolve("test_primary://foo") == "test_primary://foo?context");

    // Verify that binding another context overrides the previously-bound
    // context until the new binding is dropped.
    {
        ArResolverContext ctx2(_TestURIResolverContext("context2"));
        ArResolverContextBinder binder2(ctx2);
        TF_AXIOM(resolver.Resolve("test_primary://foo") == "test_primary://foo?context2");
    }
    TF_AXIOM(resolver.Resolve("test_primary://foo") == "test_primary://foo?context");

    // Verify that binding an unrelated context blocks the previously-bound
    // context.
    {
        ArResolverContext ctx3(ArDefaultResolverContext{});
        ArResolverContextBinder binder3(ctx3);
        TF_AXIOM(resolver.Resolve("test_primary://foo") == "test_primary://foo");
    }
    TF_AXIOM(resolver.Resolve("test_primary://foo") == "test_primary://foo?context");

    // Verify that another URI resolver that is not primary can still resolve
    ArResolverContext ctx4(_TestURIResolverContext("context4"));
    ArResolverContextBinder binder4(ctx4);
    TF_AXIOM(resolver.Resolve("test://foo") == "test://foo?context4");
}

int main(int argc, char** argv)
{
    SetupPlugins();

    printf("TestResolveWithContext ...\n");
    TestResolveWithContext();

    printf("Test PASSED\n");
    return 0;
}
