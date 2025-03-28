//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/cameraSchema.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/dirtyBitsTranslator.h"

#include "pxr/base/tf/staticTokens.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

enum _TacoDirtyBits : HdDirtyBits
{
    Clean                   = 0,
    DirtyProtein            = 1 << 0,
    DirtyTortilla           = 1 << 1,
    DirtySalsa              = 1 << 2,
    AllDirty                = (DirtyProtein
                             | DirtyTortilla
                             | DirtySalsa)
};

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (taco)
    (burger)
    (protein)
    (tortilla)
    (salsa)
);

void
_ConvertLocatorSetToDirtyBitsForTacos(
    HdDataSourceLocatorSet const& set, HdDirtyBits *bits)
{
    if (set.Intersects(HdDataSourceLocator(_tokens->taco, _tokens->protein))) {
        (*bits) |= DirtyProtein;
    }

    if (set.Intersects(HdDataSourceLocator(_tokens->taco, _tokens->tortilla))) {
        (*bits) |= DirtyTortilla;
    }

    if (set.Intersects(HdDataSourceLocator(_tokens->taco, _tokens->salsa))) {
        (*bits) |= DirtySalsa;
    }
}

void
_ConvertDirtyBitsToLocatorSetForTacos(
    const HdDirtyBits bits, HdDataSourceLocatorSet *set)
{
    if (bits & DirtyProtein) {
        set->insert(HdDataSourceLocator(_tokens->taco, _tokens->protein));
    }

    if (bits & DirtyTortilla) {
        set->insert(HdDataSourceLocator(_tokens->taco, _tokens->tortilla));
    }

    if (bits & DirtySalsa) {
        set->insert(HdDataSourceLocator(_tokens->taco, _tokens->salsa));
    }
}

bool
TestCustomSprimTypes()
{
    // This call would normally go in the type registry for something like a
    // prim adapter, render delegate or scene delegate (who might care deeply
    // about the dirtiness of tacos)
    HdDirtyBitsTranslator::RegisterTranslatorsForCustomSprimType(
        _tokens->taco,
        _ConvertLocatorSetToDirtyBitsForTacos,
        _ConvertDirtyBitsToLocatorSetForTacos);

    // confirm that dirtying an unrelated locator does not dirty a taco
    HdDataSourceLocatorSet dirtyStuff(HdCameraSchema::GetDefaultLocator());

    if (HdDirtyBitsTranslator::SprimLocatorSetToDirtyBits(
            _tokens->taco, dirtyStuff) != HdChangeTracker::Clean) {
        std::cerr << "Expected clean taco." << std::endl;
        return false;
    }

    //...and that the unknown burger type will be AllDirty
    if (HdDirtyBitsTranslator::SprimLocatorSetToDirtyBits(
            _tokens->burger, dirtyStuff) == HdChangeTracker::Clean) {
        std::cerr << "Expected dirty burger." << std::endl;
        return false;
    }

    // test round trip of bits
    HdDirtyBits bits = DirtyTortilla | DirtyProtein;
    HdDataSourceLocatorSet set;
    HdDirtyBitsTranslator::SprimDirtyBitsToLocatorSet(
        _tokens->taco, bits, &set);

    if (HdDirtyBitsTranslator::SprimLocatorSetToDirtyBits(_tokens->taco, set)
            != bits) {
        std::cerr << "Roundtrip of dirty taco doesn't match." << std::endl;
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------

#define xstr(s) str(s)
#define str(s) #s
#define TEST(X) std::cout << (++i) << ") " <<  str(X) << "..." << std::endl; \
if (!X()) { std::cout << "FAILED" << std::endl; return -1; } \
else std::cout << "...SUCCEEDED" << std::endl;

int main(int argc, char**argv)
{
    std::cout << "STARTING testHdDirtyBitsTranslator" << std::endl;
    // ------------------------------------------------------------------------

    int i = 0;
    TEST(TestCustomSprimTypes);

    // ------------------------------------------------------------------------
    std::cout << "DONE testHdDirtyBitsTranslator: SUCCESS" << std::endl;
    return 0;
}
