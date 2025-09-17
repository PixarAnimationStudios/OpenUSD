//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/arrayEdit.h"
#include "pxr/base/vt/arrayEditBuilder.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"

#include <cstdio>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

struct Equality {
    static constexpr char const *label = "equal to";
    template <class LHS, class RHS>
    static bool Test(LHS &&lhs, RHS &&rhs) {
        return std::forward<LHS>(lhs) == std::forward<RHS>(rhs);
    }
};

struct Inequality {
    static constexpr char const *label = "inequal to";
    template <class LHS, class RHS>
    static bool Test(LHS &&lhs, RHS &&rhs) {
        return std::forward<LHS>(lhs) != std::forward<RHS>(rhs);
    }
};

}

template <class Relation, class LHS, class RHS>
static bool
_CheckRelation(TfCallContext const &ctx,
               char const *lstr, char const *rstr,
               LHS &&lhs, RHS &&rhs)
{
    if (!Relation::Test(std::forward<LHS>(lhs), std::forward<RHS>(rhs))) {
        Tf_DiagnosticLiteHelper(ctx, TF_DIAGNOSTIC_FATAL_CODING_ERROR_TYPE).
            IssueFatalError("\n>> %s is not %s %s <<"
                            "\n   lhs -> %s"
                            "\n   rhs -> %s",
                            lstr, Relation::label, rstr,
                            TfStringify(lhs).c_str(),
                            TfStringify(rhs).c_str());
    }
    return true;
}

#define CHECK_EQUAL(lhs, rhs) \
    _CheckRelation<Equality>(TF_CALL_CONTEXT, #lhs, #rhs, (lhs), (rhs))

#define CHECK_INEQUAL(lhs, rhs) \
    _CheckRelation<Inequality>(TF_CALL_CONTEXT, #lhs, #rhs, (lhs), (rhs))

static void testBasics()
{
    const VtIntArray empty;
    const VtIntArrayEdit ident;

    TF_AXIOM(ident.IsIdentity());

    VtIntArrayEdit emptyDense = empty;
    TF_AXIOM(!emptyDense.IsIdentity());
    TF_AXIOM(emptyDense.IsDenseArray());
    CHECK_EQUAL(emptyDense.GetDenseArray(), VtIntArray {});

    // Ident over dense array leaves it unchanged.
    TF_AXIOM(ident.ComposeOver(empty).IsDenseArray());
    CHECK_EQUAL(ident.ComposeOver(empty).GetDenseArray(), VtIntArray {});

    VtIntArray one23 { 1, 2, 3 };
    TF_AXIOM(ident.ComposeOver(one23).IsDenseArray());
    CHECK_EQUAL(ident.ComposeOver(one23).GetDenseArray(), one23);

    // Hash
    TfHash h;
    CHECK_EQUAL(h(ident), h(VtIntArrayEdit {}));
    CHECK_EQUAL(h(emptyDense), h(VtIntArrayEdit{ VtIntArray{} }));
    CHECK_EQUAL(h(VtIntArrayEdit{ one23 }),
                h(VtIntArrayEdit{ VtIntArray { 1, 2, 3 } }));
}

static void testBuilderAndComposition()
{
    const VtIntArray empty;
    // Create an editor that prepends 0 and appends 9.
    VtIntArrayEditBuilder builder;
    {
        VtIntArrayEdit zeroNine = builder
            .Prepend(0)
            .Append(9)
            .FinalizeAndReset();

        // Composing over dense arrays.
        CHECK_EQUAL(zeroNine.ComposeOver(empty).GetDenseArray(),
                    (VtIntArray {0, 9}));
        CHECK_EQUAL(zeroNine.ComposeOver(VtIntArray { 5 }).GetDenseArray(),
                    (VtIntArray{0,5,9}));

        // Compose zeroNine itself to make a 00..99 appender.
        VtIntArrayEdit zero09Nine = zeroNine.ComposeOver(zeroNine);

        TF_AXIOM(!zero09Nine.IsDenseArray());
        CHECK_EQUAL(zero09Nine.ComposeOver(empty).GetDenseArray(),
                    (VtIntArray {0,0,9,9}));
        CHECK_EQUAL(zero09Nine.ComposeOver(VtIntArray {3,4,5}).GetDenseArray(),
                    (VtIntArray{0,0,3,4,5,9,9}));

        // Build an edit that writes the last element to index 2, the first
        // element to index 4, then erases the first and last element.
        VtIntArrayEdit mixAndTrim = builder
            .WriteRef(-1, 2)
            .WriteRef(0, 4)
            .EraseRef(-1)
            .EraseRef(0)
            .FinalizeAndReset();

        CHECK_EQUAL(
            mixAndTrim.ComposeOver(VtIntArray{0,0,3,4,5,9,9}).GetDenseArray(),
            (VtIntArray{0,9,4,0,9}));

        // Out-of-bounds operations should be ignored.
        CHECK_EQUAL(
            mixAndTrim.ComposeOver(VtIntArray{4,5,6,7}).GetDenseArray(),
            (VtIntArray{5,7}));

        VtIntArrayEdit zeroNineMixAndTrim = mixAndTrim.ComposeOver(zeroNine);
        CHECK_EQUAL(
            zeroNineMixAndTrim.ComposeOver(
                VtIntArray{1,2,3,4,5,6,7}).GetDenseArray(),
            (VtIntArray{1,9,3,0,5,6,7}));
        CHECK_EQUAL(
            zeroNineMixAndTrim.ComposeOver(VtIntArray{4,5}).GetDenseArray(),
            (VtIntArray{4,9}));

        {
            // rvalue this.
            zeroNineMixAndTrim = mixAndTrim.ComposeOver(zeroNine);
            CHECK_EQUAL(
                std::move(zeroNineMixAndTrim).ComposeOver(
                    VtIntArray{1,2,3,4,5,6,7}).GetDenseArray(),
                (VtIntArray{1,9,3,0,5,6,7}));
            zeroNineMixAndTrim = mixAndTrim.ComposeOver(zeroNine);
            CHECK_EQUAL(
                std::move(zeroNineMixAndTrim).ComposeOver(
                    VtIntArray{4,5}).GetDenseArray(),
                (VtIntArray{4,9}));

            zero09Nine = zeroNine.ComposeOver(zeroNine);
            TF_AXIOM(!zero09Nine.IsDenseArray());
            
            CHECK_EQUAL(std::move(zero09Nine).ComposeOver(
                            empty).GetDenseArray(), (VtIntArray {0,0,9,9}));
            zero09Nine = zeroNine.ComposeOver(zeroNine);
            CHECK_EQUAL(std::move(zero09Nine).ComposeOver(
                            VtIntArray {3,4,5}).GetDenseArray(),
                        (VtIntArray{0,0,3,4,5,9,9}));
        }

        VtIntArrayEdit minSize10 = builder.MinSize(10).FinalizeAndReset();
        CHECK_EQUAL(
            minSize10.ComposeOver(VtIntArray {}).GetDenseArray(),
            (VtIntArray{0,0,0,0,0,0,0,0,0,0}));
        CHECK_EQUAL(
            minSize10.ComposeOver(VtIntArray(15, 7)).GetDenseArray(),
            (VtIntArray(15, 7)));

        VtIntArrayEdit minSize10Fill9 =
            builder.MinSize(10, 9).FinalizeAndReset();
        CHECK_EQUAL(
            minSize10Fill9.ComposeOver(VtIntArray {}).GetDenseArray(),
            (VtIntArray{9,9,9,9,9,9,9,9,9,9}));

        VtIntArrayEdit maxSize15 = builder.MaxSize(15).FinalizeAndReset();
        CHECK_EQUAL(
            maxSize15.ComposeOver(VtIntArray {}).GetDenseArray(),
            (VtIntArray{}));
        CHECK_EQUAL(
            maxSize15.ComposeOver(VtIntArray(20, 2)).GetDenseArray(),
            (VtIntArray(15, 2)));

        VtIntArrayEdit size10to15 = maxSize15.ComposeOver(minSize10);
        CHECK_EQUAL(
            size10to15.ComposeOver(VtIntArray(7, 1)).GetDenseArray(),
            (VtIntArray{1,1,1,1,1,1,1,0,0,0}));
        CHECK_EQUAL(
            size10to15.ComposeOver(VtIntArray(20, 2)).GetDenseArray(),
            (VtIntArray(15, 2)));
        CHECK_EQUAL(
            size10to15.ComposeOver(VtIntArray(13, 3)).GetDenseArray(),
            (VtIntArray(13, 3)));

        VtIntArrayEdit size7 = builder.SetSize(7).FinalizeAndReset();
        CHECK_EQUAL(
            size7.ComposeOver(VtIntArray(7, 1)).GetDenseArray(),
            (VtIntArray(7, 1)));
        CHECK_EQUAL(
            size7.ComposeOver(VtIntArray {}).GetDenseArray(),
            (VtIntArray(7, 0)));
        CHECK_EQUAL(
            size7.ComposeOver(VtIntArray(27, 9)).GetDenseArray(),
            (VtIntArray(7, 9)));
        
        VtIntArrayEdit size7Fill3 = builder.SetSize(7, 3).FinalizeAndReset();
        CHECK_EQUAL(
            size7Fill3.ComposeOver(VtIntArray(7, 1)).GetDenseArray(),
            (VtIntArray(7, 1)));
        CHECK_EQUAL(
            size7Fill3.ComposeOver(VtIntArray {}).GetDenseArray(),
            (VtIntArray(7, 3)));
        CHECK_EQUAL(
            size7Fill3.ComposeOver(VtIntArray(27, 9)).GetDenseArray(),
            (VtIntArray(7, 9)));

        // Check that the serialization data will reproduce an equivalent edit.
        {
            VtIntArray vals;
            std::vector<int64_t> indexes;

            auto check = [&](VtIntArrayEdit const &test) {
                VtIntArrayEditBuilder::
                    GetSerializationData(test, &vals, &indexes);
                VtIntArrayEdit reconstituted =
                    VtIntArrayEditBuilder::CreateFromSerializationData(
                        vals, indexes, test.IsDenseArray());
                TF_AXIOM(test == reconstituted);
            };

            check(size7Fill3);
            check(size7);
            check(size10to15);
            check(minSize10Fill9);
            check(zeroNineMixAndTrim);

            check({}); // identity.
            check(VtIntArray {}); // empty dense array.
        }
    }
}

int main(int argc, char *argv[])
{
    testBasics();
    testBuilderAndComposition();

    printf("Test SUCCEEDED\n");

    return 0;
}
