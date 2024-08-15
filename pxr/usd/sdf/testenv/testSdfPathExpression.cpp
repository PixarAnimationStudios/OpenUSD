//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pathExpression.h"
#include "pxr/usd/sdf/pathExpressionEval.h"
#include "pxr/usd/sdf/pathPattern.h"
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
    SdfPredicateFunctionResult
    Match(SdfPath const &p) {
        return _eval.Match(p, PathIdentity {});
    }
    SdfPathExpressionEval<SdfPath const &> _eval;
};

} // anon

static void
TestBasics()
{
    {
        // Allow leading & trailing whitespace.
        TF_AXIOM(SdfPathExpression("  /foo//bar").GetText() == "/foo//bar");
        TF_AXIOM(SdfPathExpression("  /foo//bar ").GetText() == "/foo//bar");
        TF_AXIOM(SdfPathExpression("/foo//bar ").GetText() == "/foo//bar");
        TF_AXIOM(SdfPathExpression("  /foo /bar").GetText() == "/foo /bar");
        TF_AXIOM(SdfPathExpression("  /foo /bar ").GetText() == "/foo /bar");
        TF_AXIOM(SdfPathExpression("/foo /bar ").GetText() == "/foo /bar");
    }
    
    {
        auto eval = MatchEval { SdfPathExpression("/foo//bar") };
        
        TF_AXIOM(eval.Match(SdfPath("/foo/bar")));
        TF_AXIOM(eval.Match(SdfPath("/foo/x/bar")));
        TF_AXIOM(eval.Match(SdfPath("/foo/x/y/z/bar")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar/baz")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar.baz")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar.baz:buz")));
        TF_AXIOM(!eval.Match(SdfPath("/foo.bar")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z.bar")));
    }
    
    {
        auto eval = MatchEval { SdfPathExpression("/foo/bar/*") };
        
        TF_AXIOM(!eval.Match(SdfPath("/foo/bar"))); 
        TF_AXIOM(eval.Match(SdfPath("/foo/bar/x")));
        TF_AXIOM(eval.Match(SdfPath("/foo/bar/y")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/bar/x/y")));
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
        TF_AXIOM(!eval.Match(SdfPath("/fooY/x/y/z/bar.baz:buz")));
    }

    {
        auto eval = MatchEval { SdfPathExpression("/foo*//bar{isPrimPath}") };
        
        TF_AXIOM(eval.Match(SdfPath("/foo/bar")));
        TF_AXIOM(eval.Match(SdfPath("/foo/x/bar")));
        TF_AXIOM(eval.Match(SdfPath("/foo/x/y/z/bar")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar/baz")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar.baz")));
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar.baz:buz")));

        TF_AXIOM(eval.Match(SdfPath("/foo1/bar")));
        TF_AXIOM(eval.Match(SdfPath("/foo12/x/bar")));
        TF_AXIOM(eval.Match(SdfPath("/fooBar/x/y/z/bar")));
        TF_AXIOM(!eval.Match(SdfPath("/fooX/x/y/z/bar/baz")));
        TF_AXIOM(!eval.Match(SdfPath("/fooY/x/y/z/bar.baz")));
        TF_AXIOM(!eval.Match(SdfPath("/fooY/x/y/z/bar.baz:buz")));
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
        TF_AXIOM(!eval.Match(SdfPath("/foo/x/y/z/bar/baz/qux.ns:attr")));

        TF_AXIOM(eval.Match(SdfPath("/fooXYZ/bar/a")));
        TF_AXIOM(eval.Match(SdfPath("/fooABC/x/bar/a/b/c")));
        TF_AXIOM(eval.Match(SdfPath("/foo123/x/y/z/bar/x")));
        TF_AXIOM(eval.Match(SdfPath("/fooASDF/x/y/z/bar/baz")));
        TF_AXIOM(eval.Match(SdfPath("/foo___/x/y/z/bar/baz/qux")));
        TF_AXIOM(!eval.Match(SdfPath("/foo_bar/x/y/z/bar/baz.attr")));
        TF_AXIOM(!eval.Match(SdfPath("/foo_baz/x/y/z/bar/baz/qux.attr")));
        TF_AXIOM(!eval.Match(SdfPath("/foo_baz/x/y/z/bar/baz/qux.ns:attr")));
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
        TF_AXIOM(eval.Match(SdfPath("/a/b.ns:c")));
        TF_AXIOM(eval.Match(SdfPath("/a/b.yes")));
        TF_AXIOM(eval.Match(SdfPath("/a/b.ns:yes")));
        TF_AXIOM(!eval.Match(SdfPath("/a/b/c")));
        TF_AXIOM(eval.Match(SdfPath("/a/b/c.d")));
        TF_AXIOM(eval.Match(SdfPath("/a/b/c.ns:d")));
        TF_AXIOM(!eval.Match(SdfPath("/a/b/x")));
        TF_AXIOM(eval.Match(SdfPath("/a/b/x.y")));
        TF_AXIOM(eval.Match(SdfPath("/a/b/x.ns:y")));
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

        // ResolveReferences() with the empty expression should produce the
        // empty expression.
        TF_AXIOM(SdfPathExpression().ResolveReferences(resolveRefs).IsEmpty());
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

    {
        // Check constancy wrt prefix relations.
        auto eval = MatchEval { SdfPathExpression("/prefix/path//") };

        TF_AXIOM(!eval.Match(SdfPath("/prefix")));
        TF_AXIOM(!eval.Match(SdfPath("/prefix")).IsConstant());
        TF_AXIOM(eval.Match(SdfPath("/prefix/path")));
        TF_AXIOM(eval.Match(SdfPath("/prefix/path")).IsConstant());
        TF_AXIOM(!eval.Match(SdfPath("/prefix/wrong")));
        TF_AXIOM(eval.Match(SdfPath("/prefix/wrong")).IsConstant());
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

    auto testSearch = [&predLib, &paths](
        std::string const &exprStr,
        std::vector<std::string> const &expected) {

        auto eval = SdfMakePathExpressionEval(
            SdfPathExpression(exprStr), predLib);
        auto search = eval.MakeIncrementalSearcher(PathIdentity {});

        std::vector<std::string> matches;
        for (SdfPath const &p: paths) {
            if (search.Next(p)) {
                matches.push_back(p.GetAsString());
            }
        }
        if (matches != expected) {
            TF_FATAL_ERROR("Incremental search yielded unexpected results:\n"
                           "Expected : %s\n"
                           "Actual   : %s",
                           TfStringify(expected).c_str(),
                           TfStringify(matches).c_str());
        }
    };

    testSearch("/World",
               { "/World" });

    testSearch("/World/anim/*",
               { "/World/anim/chars",
                 "/World/anim/sets" });

    testSearch("/Foo/g*m/foo/bar",
               { "/Foo/geom/foo/bar" });

    testSearch("/Foo/g*m//foo/bar/foo",
               { "/Foo/geom/foo/bar/foo",
                 "/Foo/geom/foo/bar/foo/bar/foo" });

    testSearch("/Foo/g*m//foo//foo/bar/foo",
               { "/Foo/geom/foo/bar/foo/bar/foo" });

    testSearch("/Foo/g*m/foo//foo/bar",
               { "/Foo/geom/foo/bar/foo/bar",
                 "/Foo/geom/foo/bar/foo/bar/foo/bar" });

    testSearch("//Foo//foo/bar",
               { "/Foo/geom/foo/bar",
                 "/Foo/geom/foo/bar/foo/bar",
                 "/Foo/geom/foo/bar/foo/bar/foo/bar" });
    
    testSearch("//geom/body_sbdv",
               { "/World/anim/chars/Mike/geom/body_sbdv",
                 "/World/anim/chars/Sully/geom/body_sbdv" });

    testSearch("//chars//",
               { "/World/anim/chars",
                 "/World/anim/chars/Mike",
                 "/World/anim/chars/Mike/geom",
                 "/World/anim/chars/Mike/geom/body_sbdv",
                 "/World/anim/chars/Mike/geom/body_sbdv.points",
                 "/World/anim/chars/Sully",
                 "/World/anim/chars/Sully/geom",
                 "/World/anim/chars/Sully/geom/body_sbdv",
                 "/World/anim/chars/Sully/geom/body_sbdv.points" });

    testSearch("/World/anim/chars//",
               { "/World/anim/chars",
                 "/World/anim/chars/Mike",
                 "/World/anim/chars/Mike/geom",
                 "/World/anim/chars/Mike/geom/body_sbdv",
                 "/World/anim/chars/Mike/geom/body_sbdv.points",
                 "/World/anim/chars/Sully",
                 "/World/anim/chars/Sully/geom",
                 "/World/anim/chars/Sully/geom/body_sbdv",
                 "/World/anim/chars/Sully/geom/body_sbdv.points" });
    
    testSearch("//{isPropertyPath}",
               { "/World/anim/chars/Mike/geom/body_sbdv.points",
                 "/World/anim/chars/Sully/geom/body_sbdv.points" });

    testSearch("//chars/*/geom/body_sbdv //Bed",
               { "/World/anim/chars/Mike/geom/body_sbdv",
                 "/World/anim/chars/Sully/geom/body_sbdv",
                 "/World/anim/sets/Bedroom/Furniture/Bed" });

    testSearch("//*sbdv",
               { "/World/anim/chars/Mike/geom/body_sbdv",
                 "/World/anim/chars/Sully/geom/body_sbdv" });

    testSearch("/World//chars//geom/*sbdv",
               { "/World/anim/chars/Mike/geom/body_sbdv",
                 "/World/anim/chars/Sully/geom/body_sbdv" });

    testSearch("//*e",
               { "/World/anim/chars/Mike",
                 "/World/anim/sets/Bedroom/Furniture" });

}

static void
TestPathPattern()
{
    SdfPathPattern pat;

    TF_AXIOM(!pat);
    TF_AXIOM(!pat.HasTrailingStretch());
    TF_AXIOM(pat.GetPrefix().IsEmpty());
    TF_AXIOM(pat.CanAppendChild({})); // Can append stretch.
    TF_AXIOM(pat.AppendChild({}));
    TF_AXIOM(pat == SdfPathPattern::EveryDescendant());
    TF_AXIOM(pat.HasTrailingStretch());
    TF_AXIOM(pat.GetPrefix() == SdfPath::ReflexiveRelativePath());
    TF_AXIOM(!pat.HasLeadingStretch());

    // Set prefix to '/', should become Everything().
    pat.SetPrefix(SdfPath::AbsoluteRootPath());
    TF_AXIOM(pat == SdfPathPattern::Everything());
    TF_AXIOM(pat.HasLeadingStretch());
    TF_AXIOM(pat.HasTrailingStretch());

    // Remove trailing stretch, should become just '/'
    pat.RemoveTrailingStretch();
    TF_AXIOM(!pat.HasLeadingStretch());
    TF_AXIOM(!pat.HasTrailingStretch());
    TF_AXIOM(pat.GetPrefix() == SdfPath::AbsoluteRootPath());
    TF_AXIOM(pat.GetComponents().empty());

    // Add some components.
    pat.AppendChild("foo").AppendChild("bar").AppendChild("baz");
    // This should have modified the prefix path, rather than appending matching
    // components.
    TF_AXIOM(pat.GetPrefix() == SdfPath("/foo/bar/baz"));

    pat.AppendStretchIfPossible().AppendProperty("prop");

    // Appending a property to a pattern with trailing stretch has to append a
    // prim wildcard '*'.
    TF_AXIOM(pat.IsProperty());
    TF_AXIOM(pat.GetComponents().size() == 3);
    TF_AXIOM(pat.GetComponents()[0].text.empty());
    TF_AXIOM(pat.GetComponents()[1].text == "*");
    TF_AXIOM(pat.GetComponents()[2].text == "prop");

    TF_AXIOM(pat.GetText() == "/foo/bar/baz//*.prop");

    // Can't append children or properties to property patterns.
    TF_AXIOM(!pat.CanAppendChild("foo"));
    TF_AXIOM(!pat.CanAppendProperty("foo"));

    pat.RemoveTrailingComponent();
    TF_AXIOM(pat.GetText() == "/foo/bar/baz//*");
    pat.RemoveTrailingComponent();
    TF_AXIOM(pat.GetText() == "/foo/bar/baz//");
    pat.RemoveTrailingComponent();
    TF_AXIOM(pat.GetText() == "/foo/bar/baz");
    pat.RemoveTrailingComponent(); // No more trailing components, only prefix.
    TF_AXIOM(pat.GetText() == "/foo/bar/baz");
}

static void
TestErrors()
{
    auto expectBad = [](std::string const &exprTxt) {
        SdfPathExpression badExpr(exprTxt);
        if (!badExpr.IsEmpty()) {
            TF_FATAL_ERROR("Expected '%s' to produce the empty expression",
                           exprTxt.c_str());
        }
        if (badExpr.GetParseError().empty()) {
            TF_FATAL_ERROR("Expected parsing '%s' to yield a parse error",
                           exprTxt.c_str());
        }
    };

    fprintf(stderr, "=== Expected errors =======\n");

    expectBad("/foo///");
    expectBad("-");
    expectBad("- /foo");
    expectBad("-/foo");
    expectBad("/foo-");
    expectBad("/foo/-");
    expectBad("/foo/-/bar");
    
    fprintf(stderr, "=== End expected errors ===\n");
}


int
main(int argc, char **argv)
{
    TestBasics();
    TestSearch();
    TestPathPattern();
    TestErrors();
    
    printf(">>> Test SUCCEEDED\n");
    return 0;
}

