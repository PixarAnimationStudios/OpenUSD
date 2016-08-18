#!/pxrpythonsubst

import sys, os
from pxr import Sdf, Usd, Tf

from Mentor.Runtime import SetAssertMode, MTR_EXIT_TEST
from Mentor.Runtime.Utility import AssertEqual, AssertFalse

allFormats = ['usd' + x for x in 'abc']

def TestMilkCarton():
    myfile = ('/usr/anim/piper/devusd/inst/MilkCartonA/usd/MilkCartonA.usd')

    mylayer = Sdf.Layer.FindOrOpen(myfile)
    assert mylayer

    mystage = Usd.Stage.Open(myfile)
    assert mystage

    myprim = mystage.GetPrimAtPath('/MilkCartonA')
    assert myprim

    assert myprim.HasVariantSets()
    assert 'modelingVariant' in myprim.GetVariantSets().GetNames()
    AssertEqual(myprim.GetVariantSet('modelingVariant').GetVariantSelection(),
                'Carton_Opened')
    AssertEqual(myprim.GetVariantSets().GetVariantSelection('modelingVariant'),
                'Carton_Opened')
    AssertEqual(myprim.GetVariantSet('modelingVariant').GetVariantNames(),
                ['ALL_VARIANTS', 'Carton_Opened', 'Carton_Sealed'])
    AssertEqual(myprim.GetVariantSet('modelingVariant').GetName(),
                'modelingVariant')


def TestVariantSelectionPathAbstraction():
    for fmt in allFormats:
        s = Usd.Stage.CreateInMemory('TestVariantSelectionPathAbstraction.'+fmt)
        p = s.OverridePrim("/Foo")
        vss = p.GetVariantSets()
        assert not p.HasVariantSets()
        vs = vss.FindOrCreate("LOD")
        assert p.HasVariantSets()
        assert vs
        assert vs.FindOrCreateVariant("High")
        assert p.HasVariantSets()

        # This call triggers the bug. This happens because it triggers the
        # computation of a PcpPrimIndex for the variant prim, which then causes
        # the prim with a variant selection to be included in the UsdStage's
        # scene graph later when the next round of change processing occurs.
        #
        # XXX: WBN to indicate the bug # above.  This code changed when the
        # variant API changed during the switch to using EditTargets instead of
        # UsdPrimVariant.  It's unclear whether or not the mystery bug is still
        # reproduced. Leaving the test in place as much as possible..
        AssertFalse(p.GetAttribute("bar").IsDefined())

        # This triggers change processing which will include the prim with the
        # variant selection and put it on the stage.
        vs.SetVariantSelection('High')
        editTarget = vs.GetVariantEditTarget()
        assert editTarget
        with Usd.EditContext(s, editTarget):
            s.DefinePrim(p.GetPath().AppendChild('Foobar'), 'Scope')

        assert s.GetPrimAtPath(p.GetPath().AppendChild('Foobar'))

        # Here's the actual manifestation of the bug: We should still not have
        # this prim on the stage, but when the bug is present, we do. Paths
        # containing variant selections can never identify objects on a stage.
        # Verify that the stage does not contain a prim for the variant prim
        # spec we just created at </Foo{LOD=High}Foobar>
        testPath = p.GetPath().AppendVariantSelection(
            'LOD', 'High').AppendChild('Foobar')
        AssertFalse(s.GetPrimAtPath(testPath))

if __name__ == '__main__':
    SetAssertMode(MTR_EXIT_TEST)
    TestMilkCarton()
    TestVariantSelectionPathAbstraction()

