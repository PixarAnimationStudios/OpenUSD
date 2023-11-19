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

namespace {

struct PathIdentity {
    SdfPath operator()(SdfPath const &p) const { return p; }
};

static SdfPredicateLibrary<SdfPath const &> const &
GetBasicPredicateLib() {
    // Super simple predicate library for paths for testing.
    static auto theLib = SdfPredicateLibrary<SdfPath const &>()
        .Define("isPrimPath", [](SdfPath const &p) {
            return p.IsPrimPath();
        })
        .Define("isPropertyPath", [](SdfPath const &p) {
            return p.IsPropertyPath();
        })
        ;
    return theLib;
}

struct MatchEval {
    explicit MatchEval(SdfPathExpression const &expr)
        : _eval(SdfMakePathExpressionEval(expr, GetBasicPredicateLib())) {}
    explicit MatchEval(std::string const &exprStr) :
        MatchEval(SdfPathExpression(exprStr)) {}
    bool Match(SdfPath const &p) {
        return static_cast<bool>(
            _eval.Match(p, PathIdentity {}, PathIdentity {}));
    }
    SdfPathExpressionEval<SdfPath const &> _eval;
};

} // anon

static void
TestBasics()
{
    {
        auto eval = MatchEval { SdfPathExpression("/foo//bar") };
        
        TF_AXIOM(eval.Match(SdfPath("/foo/bar")));
        TF_AXIOM(eval.Match(SdfPath("/foo/x/bar")));
        TF_AXIOM(eval.Match(SdfPath("/foo/x/y/z/bar")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar/baz")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar.baz")));
    }
    
    {
        auto eval = MatchEval { SdfPathExpression("//foo/bar/baz/qux/quux") };
        
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
        auto eval = MatchEval { SdfPathExpression("/foo*//bar") };
        
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
        auto eval = MatchEval { SdfPathExpression("/foo*//bar{isPrimPath}") };
        
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
        auto eval = MatchEval { 
            SdfPathExpression("/foo*//bar//{isPrimPath}") };
        
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
        auto eval = MatchEval { 
            SdfPathExpression("/a /b /c /d/e/f") };

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
        auto eval = MatchEval { 
            SdfPathExpression("/a// - /a/b/c") };

        TF_AXIOM(eval.Match(SdfPath("/a")));
        TF_AXIOM(eval.Match(SdfPath("/a/b")));
        TF_AXIOM(!eval.Match(SdfPath("/a/b/c")));
        TF_AXIOM(eval.Match(SdfPath("/a/b/c/d")));
        TF_AXIOM(eval.Match(SdfPath("/a/b/x")));
        TF_AXIOM(eval.Match(SdfPath("/a/b/y")));
    }

    {
        auto eval = MatchEval { 
            SdfPathExpression("/a//{isPropertyPath} - /a/b.c") };

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
        TF_AXIOM(!a.ContainsWeakerExpressionReference());
        TF_AXIOM(b.ContainsExpressionReferences());
        TF_AXIOM(b.ContainsWeakerExpressionReference());
        TF_AXIOM(c.ContainsExpressionReferences());
        TF_AXIOM(c.ContainsWeakerExpressionReference());
        
        SdfPathExpression composed = c.ComposeOver(b).ComposeOver(a);

        TF_AXIOM(!composed.ContainsExpressionReferences());
        TF_AXIOM(!composed.ContainsWeakerExpressionReference());
        TF_AXIOM(composed.IsComplete());
        
        auto eval = MatchEval { composed };

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
        auto eval = MatchEval { resolved };

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

        auto eval = MatchEval { abs };

        TF_AXIOM(eval.Match(SdfPath("/World/test/foo")));
        TF_AXIOM(!eval.Match(SdfPath("/World/test/bar")));
        TF_AXIOM(eval.Match(SdfPath("/World/bar")));
        TF_AXIOM(eval.Match(SdfPath("/World/test/baz/qux")));
        TF_AXIOM(eval.Match(SdfPath("/World/test/baz/a/b/c/qux")));

        // ReplacePrefix.
        {
            SdfPathExpression home =
                abs.ReplacePrefix(SdfPath("/World"), SdfPath("/Home"));
            
            auto eval = MatchEval { home };

            TF_AXIOM(eval.Match(SdfPath("/Home/test/foo")));
            TF_AXIOM(!eval.Match(SdfPath("/Home/test/bar")));
            TF_AXIOM(eval.Match(SdfPath("/Home/bar")));
            TF_AXIOM(eval.Match(SdfPath("/Home/test/baz/qux")));
            TF_AXIOM(eval.Match(SdfPath("/Home/test/baz/a/b/c/qux")));
        }
    }
}


static void
TestSearch()
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

    // Paths must follow a depth-first traversal order.
    SdfPathVector paths;
    for (char const *pathStr: {
            "/",
            "/World",
            "/World/anim",
            "/World/anim/chars",
            "/World/anim/chars/Mike",
            "/World/anim/chars/Mike/geom",
            "/World/anim/chars/Mike/geom/body_sbdv",
            "/World/anim/chars/Mike/geom/body_sbdv.points",
            "/World/anim/chars/Sully",
            "/World/anim/chars/Sully/geom",
            "/World/anim/chars/Sully/geom/body_sbdv",
            "/World/anim/chars/Sully/geom/body_sbdv.points",
            "/World/anim/sets",
            "/World/anim/sets/Bedroom",
            "/World/anim/sets/Bedroom/Furniture",
            "/World/anim/sets/Bedroom/Furniture/Bed",
            "/World/anim/sets/Bedroom/Furniture/Desk",
            "/World/anim/sets/Bedroom/Furniture/Chair",
            "/Foo",
            "/Foo/geom",
            "/Foo/geom/foo",
            "/Foo/geom/foo/bar",
            "/Foo/geom/foo/bar/foo",
            "/Foo/geom/foo/bar/foo/bar",
            "/Foo/geom/foo/bar/foo/bar/foo",
            "/Foo/geom/foo/bar/foo/bar/foo/bar"
        }) {
        paths.push_back(SdfPath(pathStr));
    }

    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("/Foo/g*m/foo/bar"), predLib);
        auto search = eval.MakeIncrementalSearcher(
            PathIdentity {}, PathIdentity {});
        for (SdfPath const &p: paths) {
            if (search.Next(p)) {
                TF_AXIOM(p == SdfPath("/Foo/geom/foo/bar"));
            }
        }
    }

    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("/Foo/g*m//foo/bar/foo"), predLib);
        auto search = eval.MakeIncrementalSearcher(
            PathIdentity {}, PathIdentity {});
        for (SdfPath const &p: paths) {
            if (search.Next(p)) {
                TF_AXIOM(
                    p == SdfPath("/Foo/geom/foo/bar/foo") ||
                    p == SdfPath("/Foo/geom/foo/bar/foo/bar/foo"));
            }
        }
    }

    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("/Foo/g*m//foo//foo/bar/foo"), predLib);
        auto search = eval.MakeIncrementalSearcher(
            PathIdentity {}, PathIdentity {});
        for (SdfPath const &p: paths) {
            if (search.Next(p)) {
                TF_AXIOM(
                    p == SdfPath("/Foo/geom/foo/bar/foo/bar/foo"));
            }
        }
    }

    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("/Foo/g*m/foo//foo/bar"), predLib);
        auto search = eval.MakeIncrementalSearcher(
            PathIdentity {}, PathIdentity {});
        for (SdfPath const &p: paths) {
            if (search.Next(p)) {
                TF_AXIOM(
                    p == SdfPath("/Foo/geom/foo/bar/foo/bar") ||
                    p == SdfPath("/Foo/geom/foo/bar/foo/bar/foo/bar"));
            }
        }
    }

    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("//Foo//foo/bar"), predLib);
        auto search = eval.MakeIncrementalSearcher(
            PathIdentity {}, PathIdentity {});
        for (SdfPath const &p: paths) {
            if (search.Next(p)) {
                TF_AXIOM(
                    p == SdfPath("/Foo/geom/foo/bar") ||
                    p == SdfPath("/Foo/geom/foo/bar/foo/bar") ||
                    p == SdfPath("/Foo/geom/foo/bar/foo/bar/foo/bar"));
            }
        }
    }

    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("//geom/body_sbdv"), predLib);
        auto search = eval.MakeIncrementalSearcher(
            PathIdentity {}, PathIdentity {});
        for (SdfPath const &p: paths) {
            if (search.Next(p)) {
                TF_AXIOM(
                    p == SdfPath("/World/anim/chars/Mike/geom/body_sbdv") ||
                    p == SdfPath("/World/anim/chars/Sully/geom/body_sbdv"));
            }
        }
    }

    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("//chars//"), predLib);
        auto search = eval.MakeIncrementalSearcher(
            PathIdentity {}, PathIdentity {});
        int count = 0;
        for (SdfPath const &p: paths) {
            count += search.Next(p) ? 1 : 0;
        }
        TF_AXIOM(count == 9);
    }
    
    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("//{isPropertyPath}"), predLib);
        auto search = eval.MakeIncrementalSearcher(
            PathIdentity {}, PathIdentity {});
        int count = 0;
        for (SdfPath const &p: paths) {
            count += search.Next(p) ? 1 : 0;
        }
        TF_AXIOM(count == 2);
    }
    
    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("//chars/*/geom/body_sbdv "
                              "//Bed"), predLib);
        auto search = eval.MakeIncrementalSearcher(
            PathIdentity {}, PathIdentity {});
        for (SdfPath const &p: paths) {
            if (search.Next(p)) {
                TF_AXIOM(
                    p == SdfPath("/World/anim/chars/Mike/geom/body_sbdv") ||
                    p == SdfPath("/World/anim/chars/Sully/geom/body_sbdv") ||
                    p == SdfPath("/World/anim/sets/Bedroom/Furniture/Bed"));
            }
        }
    }

    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("//*sbdv"), predLib);
        auto search = eval.MakeIncrementalSearcher(
            PathIdentity {}, PathIdentity {});
        for (SdfPath const &p: paths) {
            if (search.Next(p)) {
                TF_AXIOM(
                    p == SdfPath("/World/anim/chars/Mike/geom/body_sbdv") ||
                    p == SdfPath("/World/anim/chars/Sully/geom/body_sbdv"));
            }
        }
    }
    {
        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression("/World//chars//geom/*sbdv"), predLib);
        auto search = eval.MakeIncrementalSearcher(
            PathIdentity {}, PathIdentity {});
        for (SdfPath const &p: paths) {
            if (search.Next(p)) {
                TF_AXIOM(
                    p == SdfPath("/World/anim/chars/Mike/geom/body_sbdv") ||
                    p == SdfPath("/World/anim/chars/Sully/geom/body_sbdv"));
            }
        }
    }
}


int
main(int argc, char **argv)
{
    TestBasics();
    TestSearch();
    
    printf(">>> Test SUCCEEDED\n");
    return 0;
}

