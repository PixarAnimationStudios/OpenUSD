//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/setenv.h"
#include "pxr/base/tf/status.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_USING_DIRECTIVE;

static void
SetupPlugins()
{
    // Set the preferred resolver to ArDefaultResolver before
    // running any test cases.
    ArSetPreferredResolver("ArDefaultResolver");

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
    TF_AXIOM(resolver.Resolve("test://foo") == "test://foo?context");

    // Verify that binding another context overrides the previously-bound
    // context until the new binding is dropped.
    {
        ArResolverContext ctx2(_TestURIResolverContext("context2"));
        ArResolverContextBinder binder2(ctx2);
        TF_AXIOM(resolver.Resolve("test://foo") == "test://foo?context2");
    }
    TF_AXIOM(resolver.Resolve("test://foo") == "test://foo?context");

    // Verify that binding an unrelated context blocks the previously-bound
    // context.
    {
        ArResolverContext ctx3(ArDefaultResolverContext{});
        ArResolverContextBinder binder3(ctx3);
        TF_AXIOM(resolver.Resolve("test://foo") == "test://foo");
    }
    TF_AXIOM(resolver.Resolve("test://foo") == "test://foo?context");
}

static void
TestCreateContextFromString()
{
    ArResolver& resolver = ArGetResolver();

    const std::vector<std::string> searchPaths = { "/a", "/b" };
    const std::string searchPathStr = 
        TfStringJoin(searchPaths, ARCH_PATH_LIST_SEP);

    // CreateContextFromString with an empty URI scheme should be
    // equivalent to CreateContextFromString without a URI scheme.
    TF_AXIOM(
        resolver.CreateContextFromString("", searchPathStr) ==
        ArResolverContext(ArDefaultResolverContext(searchPaths)));

    TF_AXIOM(
        resolver.CreateContextFromString("", searchPathStr) ==
        resolver.CreateContextFromString(searchPathStr));

    // CreateContextFromString with a URI scheme that has no registered
    // resolver results in an empty ArResolverContext.
    TF_AXIOM(
        resolver.CreateContextFromString("bogus", "context string") ==
        ArResolverContext());
    
    // CreateContextFromString with a URI scheme with a registered resolver
    // results in whatever context is returned from that resolver.
    TF_AXIOM(
        resolver.CreateContextFromString("test", "context string") ==
        ArResolverContext(_TestURIResolverContext("context string")));

    // CreateContextFromStrings should return a single ArResolverContext
    // containing context objects based on the given URI schemes and
    // context strings.
    TF_AXIOM(
        resolver.CreateContextFromStrings({
            {"test", "context string"}
        }) ==
        ArResolverContext(
            _TestURIResolverContext("context string")
        ));

    TF_AXIOM(
        resolver.CreateContextFromStrings({
            {"", TfStringJoin(searchPaths, ARCH_PATH_LIST_SEP)},
            {"test", "context string"},
            {"bogus", "context string" }
        }) ==
        ArResolverContext(
            ArDefaultResolverContext(searchPaths),
            _TestURIResolverContext("context string")));
}

static void
TestCreateDefaultContext()
{
    ArResolver& resolver = ArGetResolver();

    // CreateDefaultContext should return an ArResolverContext containing
    // the union of the default contexts returned by all registered resolvers.
    // ArDefaultResolver returns an empty ArResolverContext object as its
    // default so we can't test for that, but TestArURIResolver returns a
    // _TestURIResolverContext which we can check for here.
    const ArResolverContext defaultContext = resolver.CreateDefaultContext();

    const _TestURIResolverContext* uriCtx =
        defaultContext.Get<_TestURIResolverContext>();
    TF_AXIOM(uriCtx);
    TF_AXIOM(uriCtx->data == "CreateDefaultContext");
}

static void
TestCreateDefaultContextForAsset()
{
    auto runTest = [](const std::string& assetPath) {
        ArResolver& resolver = ArGetResolver();

        // CreateDefaultContextForAsset should return an ArResolverContext
        // containing the union of the default contexts returned by all
        // registered resolvers.
        const ArResolverContext defaultContext = 
            resolver.CreateDefaultContextForAsset(assetPath);

        // ArDefaultResolver returns an ArDefaultResolverContext whose search
        // path is set to the directory of the asset.
        {
            const ArDefaultResolverContext* defaultCtx =
                defaultContext.Get<ArDefaultResolverContext>();
            TF_AXIOM(defaultCtx);

            const ArDefaultResolverContext expectedCtx(
                std::vector<std::string>{ 
                    TfGetPathName(TfAbsPath(assetPath)) });
            TF_AXIOM(*defaultCtx == expectedCtx);
        }

        // TestArURIResolver returns a _TestURIResolverContext whose data field
        // is set to the absolute path of the asset.
        {
            const _TestURIResolverContext* uriCtx =
                defaultContext.Get<_TestURIResolverContext>();
            TF_AXIOM(uriCtx);
        
            const _TestURIResolverContext expectedCtx(
                TfAbsPath("test/test.file"));
            TF_AXIOM(*uriCtx == expectedCtx);
        }
    };

    runTest("test/test.file");

    // For a package-relative path, CreateDefaultContextForAsset should only
    // consider the outer-most package path.
    runTest("test/test.file[in_package]");
}

int main(int argc, char** argv)
{
    SetupPlugins();

    printf("TestResolveWithContext ...\n");
    TestResolveWithContext();

    printf("TestCreateContextFromString ...\n");
    TestCreateContextFromString();

    printf("TestCreateDefaultContext ...\n");
    TestCreateDefaultContext();

    printf("TestCreateDefaultContextForAsset ...\n");
    TestCreateDefaultContextForAsset();

    printf("Test PASSED\n");
    return 0;
}
