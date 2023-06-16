//
// Copyright 2023 Pixar
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
#include "pxr/base/tf/errorMark.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pathExpression.h"
#include "pxr/usd/sdf/pathExpressionEval.h"
#include "pxr/usd/sdf/predicateLibrary.h"

PXR_NAMESPACE_USING_DIRECTIVE

// For testing, provide overloads of SdfPathExpressionObjectToPath &
// PathToObject so that just return the path itself.
PXR_NAMESPACE_OPEN_SCOPE

static SdfPath
SdfPathExpressionObjectToPath(SdfPath const &path) {
    return path;
}

static SdfPath
SdfPathExpressionPathToObject(SdfPath const &path, SdfPath const *) {
    return path;
}

PXR_NAMESPACE_CLOSE_SCOPE

static void
TestBasics()
{
    // Super simple predicate library for paths for testing.
    auto predLib = SdfPredicateLibrary<SdfPath const &>()
        .Define("isPrimPath", [](SdfPath const &p) {
            return p.IsPrimPath();
        })
        .Define("isPropertyPath", [](SdfPath const &p) {
            return p.IsPropertyPath();
        })
        ;

    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("/foo//bar"), predLib);
        
        TF_AXIOM(eval.Match(SdfPath("/foo/bar")));
        TF_AXIOM(eval.Match(SdfPath("/foo/x/bar")));
        TF_AXIOM(eval.Match(SdfPath("/foo/x/y/z/bar")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar/baz")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar.baz")));
    }
    
    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("//foo/bar/baz/qux/quux"),
            predLib);
        
        TF_AXIOM(!eval.Match(SdfPath("/foo")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/bar")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/bar/baz")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/bar/baz/qux")));

        TF_AXIOM(eval.Match(SdfPath("/foo/bar/baz/qux/quux")));
        TF_AXIOM(eval.Match(SdfPath("/foo/foo/bar/baz/qux/quux")));
        TF_AXIOM(eval.Match(SdfPath("/foo/bar/foo/bar/baz/qux/quux")));
        TF_AXIOM(eval.Match(SdfPath("/foo/bar/baz/foo/bar/baz/qux/quux")));
        TF_AXIOM(eval.Match(SdfPath("/foo/bar/baz/qux/foo/bar/baz/qux/quux")));
    }

    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("/foo*//bar"), predLib);
        
        TF_AXIOM(eval.Match(SdfPath("/foo/bar")));
        TF_AXIOM(eval.Match(SdfPath("/foo/x/bar")));
        TF_AXIOM(eval.Match(SdfPath("/foo/x/y/z/bar")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar/baz")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar.baz")));

        TF_AXIOM(eval.Match(SdfPath("/foo1/bar")));
        TF_AXIOM(eval.Match(SdfPath("/foo12/x/bar")));
        TF_AXIOM(eval.Match(SdfPath("/fooBar/x/y/z/bar")));
        TF_AXIOM(!eval.Match(SdfPath("/fooX/x/y/z/bar/baz")));
        TF_AXIOM(!eval.Match(SdfPath("/fooY/x/y/z/bar.baz")));
    }

    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("/foo*//bar{isPrimPath}"),
            predLib);
        
        TF_AXIOM(eval.Match(SdfPath("/foo/bar")));
        TF_AXIOM(eval.Match(SdfPath("/foo/x/bar")));
        TF_AXIOM(eval.Match(SdfPath("/foo/x/y/z/bar")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar/baz")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar.baz")));

        TF_AXIOM(eval.Match(SdfPath("/foo1/bar")));
        TF_AXIOM(eval.Match(SdfPath("/foo12/x/bar")));
        TF_AXIOM(eval.Match(SdfPath("/fooBar/x/y/z/bar")));
        TF_AXIOM(!eval.Match(SdfPath("/fooX/x/y/z/bar/baz")));
        TF_AXIOM(!eval.Match(SdfPath("/fooY/x/y/z/bar.baz")));
    }

    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("/foo*//bar//{isPrimPath}"),
            predLib);
        
        TF_AXIOM(eval.Match(SdfPath("/foo/bar/a")));
        TF_AXIOM(eval.Match(SdfPath("/foo/x/bar/b")));
        TF_AXIOM(eval.Match(SdfPath("/foo/x/y/z/bar/c")));
        TF_AXIOM(eval.Match(SdfPath("/foo/x/y/z/bar/baz")));
        TF_AXIOM(eval.Match(SdfPath("/foo/x/y/z/bar/baz/qux")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar/baz.attr")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar/baz/qux.attr")));

        TF_AXIOM(eval.Match(SdfPath("/fooXYZ/bar/a")));
        TF_AXIOM(eval.Match(SdfPath("/fooABC/x/bar/a/b/c")));
        TF_AXIOM(eval.Match(SdfPath("/foo123/x/y/z/bar/x")));
        TF_AXIOM(eval.Match(SdfPath("/fooASDF/x/y/z/bar/baz")));
        TF_AXIOM(eval.Match(SdfPath("/foo___/x/y/z/bar/baz/qux")));
        TF_AXIOM(!eval.Match(SdfPath("/foo_bar/x/y/z/bar/baz.attr")));
        TF_AXIOM(!eval.Match(SdfPath("/foo_baz/x/y/z/bar/baz/qux.attr")));
    }
    
    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("/a /b /c /d/e/f"),
            predLib);

        TF_AXIOM(eval.Match(SdfPath("/a")));
        TF_AXIOM(eval.Match(SdfPath("/b")));
        TF_AXIOM(eval.Match(SdfPath("/c")));
        TF_AXIOM(eval.Match(SdfPath("/d/e/f")));

        TF_AXIOM(!eval.Match(SdfPath("/a/b")));
        TF_AXIOM(!eval.Match(SdfPath("/b/c")));
        TF_AXIOM(!eval.Match(SdfPath("/c/d")));
        TF_AXIOM(!eval.Match(SdfPath("/d/e")));
    }

    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("/a// - /a/b/c"),
            predLib);

        TF_AXIOM(eval.Match(SdfPath("/a")));
        TF_AXIOM(eval.Match(SdfPath("/a/b")));
        TF_AXIOM(!eval.Match(SdfPath("/a/b/c")));
        TF_AXIOM(eval.Match(SdfPath("/a/b/c/d")));
        TF_AXIOM(eval.Match(SdfPath("/a/b/x")));
        TF_AXIOM(eval.Match(SdfPath("/a/b/y")));
    }

    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("/a//{isPropertyPath} - /a/b.c"),
            predLib);

        TF_AXIOM(!eval.Match(SdfPath("/a")));
        TF_AXIOM(eval.Match(SdfPath("/a.b")));
        TF_AXIOM(!eval.Match(SdfPath("/a/b")));
        TF_AXIOM(!eval.Match(SdfPath("/a/b.c")));
        TF_AXIOM(eval.Match(SdfPath("/a/b.yes")));
        TF_AXIOM(!eval.Match(SdfPath("/a/b/c")));
        TF_AXIOM(eval.Match(SdfPath("/a/b/c.d")));
        TF_AXIOM(!eval.Match(SdfPath("/a/b/x")));
        TF_AXIOM(eval.Match(SdfPath("/a/b/x.y")));
    }

    {
        // ComposeOver
        SdfPathExpression a("/a");
        SdfPathExpression b("%_ /b");
        SdfPathExpression c("%_ /c");

        TF_AXIOM(!a.ContainsExpressionReferences());
        TF_AXIOM(b.ContainsExpressionReferences());
        TF_AXIOM(c.ContainsExpressionReferences());
        
        SdfPathExpression composed = c.ComposeOver(b).ComposeOver(a);

        TF_AXIOM(!composed.ContainsExpressionReferences());
        TF_AXIOM(composed.IsComplete());
        
        auto eval = SdfMakePathExpressionEval(
            composed, predLib);

        TF_AXIOM(eval.Match(SdfPath("/a")));
        TF_AXIOM(eval.Match(SdfPath("/b")));
        TF_AXIOM(eval.Match(SdfPath("/c")));
        TF_AXIOM(!eval.Match(SdfPath("/d")));
    }

    {
        // ResolveReferences
        SdfPathExpression refs("/a %_ %:foo - %:bar");
        SdfPathExpression weaker("/weaker");
        SdfPathExpression foo("/foo//");
        SdfPathExpression bar("/foo/bar//");

        TF_AXIOM(refs.ContainsExpressionReferences());
        TF_AXIOM(!weaker.ContainsExpressionReferences());
        TF_AXIOM(!foo.ContainsExpressionReferences());
        TF_AXIOM(!bar.ContainsExpressionReferences());

        auto resolveRefs =
            [&](SdfPathExpression::ExpressionReference const &ref) {
                if (ref.name == "_") { return weaker; }
                else if (ref.name == "foo") { return foo; }
                else if (ref.name == "bar") { return bar; }
                return SdfPathExpression {};
            };

        SdfPathExpression resolved = refs.ResolveReferences(resolveRefs);

        TF_AXIOM(!resolved.ContainsExpressionReferences());
        TF_AXIOM(resolved.IsComplete());

        // Resolved should be "/a /weaker /foo// - /foo/bar//"
        auto eval = SdfMakePathExpressionEval(
            resolved, predLib);

        TF_AXIOM(eval.Match(SdfPath("/a")));
        TF_AXIOM(eval.Match(SdfPath("/weaker")));
        TF_AXIOM(eval.Match(SdfPath("/foo")));
        TF_AXIOM(eval.Match(SdfPath("/foo/child")));
        TF_AXIOM(!eval.Match(SdfPath("/a/b")));
        TF_AXIOM(!eval.Match(SdfPath("/weaker/c")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/bar")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/bar/baz")));
    }

    {
        // Check MakeAbsolute.
        SdfPathExpression e("foo ../bar baz//qux");
        TF_AXIOM(!e.IsAbsolute());
        TF_AXIOM(!e.ContainsExpressionReferences());
        SdfPathExpression abs = e.MakeAbsolute(SdfPath("/World/test"));
        // abs should be: "/World/test/foo /World/bar /World/test/baz//qux"
        TF_AXIOM(abs.IsAbsolute());
        TF_AXIOM(abs.IsComplete());

        auto eval = SdfMakePathExpressionEval(abs, predLib);

        TF_AXIOM(eval.Match(SdfPath("/World/test/foo")));
        TF_AXIOM(!eval.Match(SdfPath("/World/test/bar")));
        TF_AXIOM(eval.Match(SdfPath("/World/bar")));
        TF_AXIOM(eval.Match(SdfPath("/World/test/baz/qux")));
        TF_AXIOM(eval.Match(SdfPath("/World/test/baz/a/b/c/qux")));

        // ReplacePrefix.
        {
            SdfPathExpression home =
                abs.ReplacePrefix(SdfPath("/World"), SdfPath("/Home"));
            
            auto eval =
                SdfMakePathExpressionEval(home, predLib);

            TF_AXIOM(eval.Match(SdfPath("/Home/test/foo")));
            TF_AXIOM(!eval.Match(SdfPath("/Home/test/bar")));
            TF_AXIOM(eval.Match(SdfPath("/Home/bar")));
            TF_AXIOM(eval.Match(SdfPath("/Home/test/baz/qux")));
            TF_AXIOM(eval.Match(SdfPath("/Home/test/baz/a/b/c/qux")));
        }
    }
}

int
main(int argc, char **argv)
{
    TestBasics();
    
    printf(">>> Test SUCCEEDED\n");
    return 0;
}

