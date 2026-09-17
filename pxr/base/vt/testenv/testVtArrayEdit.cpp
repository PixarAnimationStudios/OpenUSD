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

// Check that Optimize() produces an edit that acts identically to its input
// over a range of array sizes.  Optimize() is only permitted to change an
// edit's representation, never its behavior.
static void
_CheckOptimizeEquivalent(TfCallContext const &ctx,
                         char const *editStr, VtIntArrayEdit const &edit)
{
    const VtIntArray inputs[] = {
        VtIntArray {},
        VtIntArray {1},
        VtIntArray {1, 2, 3},
        VtIntArray {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
        VtIntArray(20, 5),
    };

    const VtIntArrayEdit opt =
        VtIntArrayEditBuilder::Optimize(VtIntArrayEdit {edit});

    for (VtIntArray const &input: inputs) {
        const VtIntArray expected = edit.ComposeOver(input);
        const VtIntArray actual = opt.ComposeOver(input);
        if (expected != actual) {
            Tf_DiagnosticLiteHelper(
                ctx, TF_DIAGNOSTIC_FATAL_CODING_ERROR_TYPE).
                IssueFatalError("\n>> Optimize(%s) does not act identically <<"
                                "\n   edit      -> %s"
                                "\n   optimized -> %s"
                                "\n   input     -> %s"
                                "\n   expected  -> %s"
                                "\n   actual    -> %s",
                                editStr,
                                TfStringify(edit).c_str(),
                                TfStringify(opt).c_str(),
                                TfStringify(input).c_str(),
                                TfStringify(expected).c_str(),
                                TfStringify(actual).c_str());
        }
    }
}

#define CHECK_OPTIMIZE_EQUIVALENT(edit) \
    _CheckOptimizeEquivalent(TF_CALL_CONTEXT, #edit, (edit))

static void testBasics()
{
    const VtIntArray empty;
    const VtIntArrayEdit ident;

    TF_AXIOM(ident.IsIdentity());

    // Ident over dense array leaves it unchanged.
    CHECK_EQUAL(ident.ComposeOver(empty), VtIntArray {});

    VtIntArray one23 { 1, 2, 3 };
    CHECK_EQUAL(ident.ComposeOver(one23), one23);

    // Hash
    TfHash h;
    CHECK_EQUAL(h(ident), h(VtIntArrayEdit {}));
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
        CHECK_EQUAL(zeroNine.ComposeOver(empty), (VtIntArray {0, 9}));
        CHECK_EQUAL(zeroNine.ComposeOver(VtIntArray {5}), (VtIntArray{0,5,9}));

        // Compose zeroNine itself to make a 00..99 appender.
        VtIntArrayEdit zero09Nine = zeroNine.ComposeOver(zeroNine);

        CHECK_EQUAL(zero09Nine.ComposeOver(empty), (VtIntArray {0,0,9,9}));
        CHECK_EQUAL(zero09Nine.ComposeOver(VtIntArray {3,4,5}),
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
            mixAndTrim.ComposeOver(VtIntArray{0,0,3,4,5,9,9}),
            (VtIntArray{0,9,4,0,9}));

        // Out-of-bounds operations should be ignored.
        CHECK_EQUAL(
            mixAndTrim.ComposeOver(VtIntArray{4,5,6,7}), (VtIntArray{5,7}));

        VtIntArrayEdit zeroNineMixAndTrim = mixAndTrim.ComposeOver(zeroNine);
        CHECK_EQUAL(
            zeroNineMixAndTrim.ComposeOver(
                VtIntArray{1,2,3,4,5,6,7}), (VtIntArray{1,9,3,0,5,6,7}));
        CHECK_EQUAL(
            zeroNineMixAndTrim.ComposeOver(VtIntArray{4,5}), (VtIntArray{4,9}));

        {
            // rvalue this.
            zeroNineMixAndTrim = mixAndTrim.ComposeOver(zeroNine);
            CHECK_EQUAL(
                std::move(zeroNineMixAndTrim).ComposeOver(
                    VtIntArray{1,2,3,4,5,6,7}), (VtIntArray{1,9,3,0,5,6,7}));
            zeroNineMixAndTrim = mixAndTrim.ComposeOver(zeroNine);
            CHECK_EQUAL(
                std::move(zeroNineMixAndTrim).ComposeOver(
                    VtIntArray{4,5}), (VtIntArray{4,9}));

            zero09Nine = zeroNine.ComposeOver(zeroNine);
            
            CHECK_EQUAL(std::move(zero09Nine).ComposeOver(empty),
                        (VtIntArray {0,0,9,9}));
            zero09Nine = zeroNine.ComposeOver(zeroNine);
            CHECK_EQUAL(std::move(zero09Nine).ComposeOver(
                            VtIntArray {3,4,5}), (VtIntArray{0,0,3,4,5,9,9}));
        }

        VtIntArrayEdit minSize10 = builder.MinSize(10).FinalizeAndReset();
        CHECK_EQUAL(
            minSize10.ComposeOver(VtIntArray {}),
            (VtIntArray{0,0,0,0,0,0,0,0,0,0}));
        CHECK_EQUAL(
            minSize10.ComposeOver(VtIntArray(15, 7)),
            (VtIntArray(15, 7)));

        VtIntArrayEdit minSize10Fill9 =
            builder.MinSize(10, 9).FinalizeAndReset();
        CHECK_EQUAL(
            minSize10Fill9.ComposeOver(VtIntArray {}),
            (VtIntArray{9,9,9,9,9,9,9,9,9,9}));

        VtIntArrayEdit maxSize15 = builder.MaxSize(15).FinalizeAndReset();
        CHECK_EQUAL(
            maxSize15.ComposeOver(VtIntArray {}),
            (VtIntArray{}));
        CHECK_EQUAL(
            maxSize15.ComposeOver(VtIntArray(20, 2)),
            (VtIntArray(15, 2)));

        VtIntArrayEdit size10to15 = maxSize15.ComposeOver(minSize10);
        CHECK_EQUAL(
            size10to15.ComposeOver(VtIntArray(7, 1)),
            (VtIntArray{1,1,1,1,1,1,1,0,0,0}));
        CHECK_EQUAL(
            size10to15.ComposeOver(VtIntArray(20, 2)),
            (VtIntArray(15, 2)));
        CHECK_EQUAL(
            size10to15.ComposeOver(VtIntArray(13, 3)),
            (VtIntArray(13, 3)));

        VtIntArrayEdit size7 = builder.SetSize(7).FinalizeAndReset();
        CHECK_EQUAL(
            size7.ComposeOver(VtIntArray(7, 1)),
            (VtIntArray(7, 1)));
        CHECK_EQUAL(
            size7.ComposeOver(VtIntArray {}),
            (VtIntArray(7, 0)));
        CHECK_EQUAL(
            size7.ComposeOver(VtIntArray(27, 9)),
            (VtIntArray(7, 9)));
        
        VtIntArrayEdit size7Fill3 = builder.SetSize(7, 3).FinalizeAndReset();
        CHECK_EQUAL(
            size7Fill3.ComposeOver(VtIntArray(7, 1)),
            (VtIntArray(7, 1)));
        CHECK_EQUAL(
            size7Fill3.ComposeOver(VtIntArray {}),
            (VtIntArray(7, 3)));
        CHECK_EQUAL(
            size7Fill3.ComposeOver(VtIntArray(27, 9)),
            (VtIntArray(7, 9)));

        // Check that the serialization data will reproduce an equivalent edit.
        {
            VtIntArray vals;
            std::vector<int64_t> indexes;

            auto check = [&](VtIntArrayEdit const &test) {
                VtIntArrayEditBuilder::
                    GetSerializationData(test, &vals, &indexes);
                VtIntArrayEdit reconstituted = VtIntArrayEditBuilder::
                    CreateFromSerializationData(vals, indexes);
                TF_AXIOM(test == reconstituted);
            };

            check(size7Fill3);
            check(size7);
            check(size10to15);
            check(minSize10Fill9);
            check(zeroNineMixAndTrim);

            check({}); // identity.
        }
    }

    // Formerly buggy case.
    {
        VtIntArrayEditBuilder builder;
        VtIntArrayEdit zeroNine = builder
            .Prepend(0)
            .Append(9)
            .FinalizeAndReset();

        VtIntArrayEdit oneEight = builder
            .Prepend(1)
            .Append(8)
            .FinalizeAndReset();

        VtIntArrayEdit twoSeven = builder
            .Prepend(2)
            .Append(7)
            .FinalizeAndReset();

        VtIntArrayEdit composed =
            zeroNine.ComposeOver(
                oneEight.ComposeOver(
                    twoSeven));

        VtIntArray a = composed.ComposeOver(empty);

        CHECK_EQUAL(a, (VtIntArray { 0, 1, 2, 7, 8, 9 }));
    }
}

// The MinSizeFill and SetSizeFill ops store their literal index in their
// *second* argument, unlike WriteLiteral and InsertLiteral which store it in
// their first.  Composition has to rebase the stronger edit's literal indexes
// past the weaker edit's literals, so it must know which argument holds the
// index for each op.
static void testComposeFillLiterals()
{
    VtIntArrayEditBuilder builder;

    // literals = {7}, ops = MinSizeFill(size = 5, literal = 0)
    const VtIntArrayEdit minSize5Fill7 =
        builder.MinSize(5, 7).FinalizeAndReset();

    // A weaker edit that also carries a literal, so composing over it must
    // shift the stronger edit's literal indexes by one.
    // literals = {3}, ops = InsertLiteral(literal = 0, index = 0)
    const VtIntArrayEdit prepend3 = builder.Prepend(3).FinalizeAndReset();

    // Prepend 3 to the empty array giving {3}, then grow to size 5 filling
    // with 7.
    const VtIntArray expectMinSize {3, 7, 7, 7, 7};

    // There are two distinct _ComposeEdits() implementations, one taking the
    // weaker edit by const reference and one by rvalue reference.  Both do
    // their own literal rebasing, so exercise both.
    CHECK_EQUAL(minSize5Fill7.ComposeOver(prepend3).ComposeOver(VtIntArray {}),
                expectMinSize);
    CHECK_EQUAL(minSize5Fill7.ComposeOver(VtIntArrayEdit {prepend3})
                .ComposeOver(VtIntArray {}),
                expectMinSize);

    // Same, for SetSizeFill.
    const VtIntArrayEdit size4Fill8 =
        builder.SetSize(4, 8).FinalizeAndReset();
    const VtIntArrayEdit prepend2 = builder.Prepend(2).FinalizeAndReset();

    const VtIntArray expectSetSize {2, 8, 8, 8};

    CHECK_EQUAL(size4Fill8.ComposeOver(prepend2).ComposeOver(VtIntArray {}),
                expectSetSize);
    CHECK_EQUAL(size4Fill8.ComposeOver(VtIntArrayEdit {prepend2})
                .ComposeOver(VtIntArray {}),
                expectSetSize);
}

// Composing an edit over itself.  The rvalue-this overloads move from *this
// before reading the weaker edit, so they have to detect the aliasing and
// compose over a copy.
static void testSelfComposition()
{
    VtIntArrayEditBuilder builder;

    const VtIntArrayEdit zeroNine =
        builder.Prepend(0).Append(9).FinalizeAndReset();
    const VtIntArray expected {0, 0, 9, 9};

    // const-lvalue this, which copies *this first and so is safe.
    CHECK_EQUAL(zeroNine.ComposeOver(zeroNine).ComposeOver(VtIntArray {}),
                expected);

    // rvalue this, const-lvalue weaker.
    VtIntArrayEdit a = zeroNine;
    CHECK_EQUAL(std::move(a).ComposeOver(a).ComposeOver(VtIntArray {}),
                expected);

    // rvalue this, rvalue weaker.
    VtIntArrayEdit b = zeroNine;
    CHECK_EQUAL(std::move(b).ComposeOver(std::move(b))
                .ComposeOver(VtIntArray {}),
                expected);
}

// Applying an edit to an rvalue array must edit that array's buffer in place
// rather than copying it.  The edit here only writes, so no reallocation can
// occur and the buffer address must survive.
//
// Comparing raw data pointers looks like a workaround for
// VtArray::IsIdentical(), but IsIdentical() cannot express this property.  It
// needs a second array to compare against, and any such array is itself a
// copy-on-write reference, which makes the buffer non-unique and so causes the
// very detach this test aims to rule out; the comparison would fail even when
// the behavior is correct.  A raw pointer records the address without
// participating in the reference count, which is what makes it usable here.
//
// Asserting that the source array was left empty would be another approach, but
// VtArray does not guarantee the state of a moved-from array.
static void testApplyToRvalueArrayDoesNotCopy()
{
    VtIntArrayEditBuilder builder;
    const VtIntArrayEdit write5At0 = builder.Write(5, 0).FinalizeAndReset();

    VtIntArray array(100, 1);
    int const *before = array.cdata();

    const VtIntArray result = write5At0.ComposeOver(std::move(array));

    CHECK_EQUAL(result[0], 5);
    TF_AXIOM(result.size() == 100);

    // A differing address means a copy-on-write detach copied the array.
    TF_AXIOM(result.cdata() == before);

    // The const-reference overload must copy, leaving the caller's array alone.
    const VtIntArray shared(100, 1);
    const VtIntArray fromConst = write5At0.ComposeOver(shared);
    CHECK_EQUAL(fromConst[0], 5);
    CHECK_EQUAL(shared[0], 1);
}

static void testOptimize()
{
    VtIntArrayEditBuilder builder;

    CHECK_OPTIMIZE_EQUIVALENT(VtIntArrayEdit {}); // identity

    CHECK_OPTIMIZE_EQUIVALENT(builder.Prepend(0).Append(9).FinalizeAndReset());

    CHECK_OPTIMIZE_EQUIVALENT(builder.MinSize(10).FinalizeAndReset());
    CHECK_OPTIMIZE_EQUIVALENT(builder.MaxSize(15).FinalizeAndReset());
    CHECK_OPTIMIZE_EQUIVALENT(builder.SetSize(7).FinalizeAndReset());

    CHECK_OPTIMIZE_EQUIVALENT(builder
                              .WriteRef(-1, 2)
                              .WriteRef(0, 4)
                              .EraseRef(-1)
                              .EraseRef(0)
                              .FinalizeAndReset());

    // Fill variants.  Note the size argument exceeds the number of literals in
    // both of these, which is the ordinary case.
    CHECK_OPTIMIZE_EQUIVALENT(builder.MinSize(10, 9).FinalizeAndReset());
    CHECK_OPTIMIZE_EQUIVALENT(builder.SetSize(7, 3).FinalizeAndReset());

    // A fill op whose size argument happens to be smaller than the number of
    // literals, which is the boundary case for a bounds check that confuses the
    // size argument for the literal index.
    CHECK_OPTIMIZE_EQUIVALENT(builder
                              .Write(11, 0)
                              .Write(12, 1)
                              .Write(13, 2)
                              .MinSize(2, 9)
                              .FinalizeAndReset());

    // Composed edits, which is what Optimize() is for.
    {
        const VtIntArrayEdit zeroNine =
            builder.Prepend(0).Append(9).FinalizeAndReset();
        const VtIntArrayEdit fill7 = builder.MinSize(5, 7).FinalizeAndReset();

        CHECK_OPTIMIZE_EQUIVALENT(zeroNine.ComposeOver(zeroNine));
        CHECK_OPTIMIZE_EQUIVALENT(fill7.ComposeOver(zeroNine));
        CHECK_OPTIMIZE_EQUIVALENT(zeroNine.ComposeOver(fill7));
    }

    // Optimizing a non-identity edit must not produce the identity.
    {
        const VtIntArrayEdit minSize10Fill9 =
            builder.MinSize(10, 9).FinalizeAndReset();
        TF_AXIOM(!minSize10Fill9.IsIdentity());
        TF_AXIOM(!VtIntArrayEditBuilder::Optimize(
                     VtIntArrayEdit {minSize10Fill9}).IsIdentity());
    }
}

int main(int argc, char *argv[])
{
    testBasics();
    testBuilderAndComposition();
    testComposeFillLiterals();
    testSelfComposition();
    testApplyToRvalueArrayDoesNotCopy();
    testOptimize();

    printf("Test SUCCEEDED\n");

    return 0;
}
