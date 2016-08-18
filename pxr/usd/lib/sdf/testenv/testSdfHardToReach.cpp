#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/notice.h"

static void
_TestSdfLayerDictKeyOps()
{
    std::string txt;

    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    SdfPath fooPath("/foo");
    SdfPrimSpecHandle foo = SdfCreatePrimInLayer(layer, fooPath);

    // Set a key in a nested dict.
    layer->SetFieldDictValueByKey(fooPath, SdfFieldKeys->CustomData,
                                  TfToken("hello:world"), std::string("value"));

    // Obtain the whole dict and check the key was set correctly.
    VtValue dictVal = layer->GetField(fooPath, SdfFieldKeys->CustomData);
    TF_AXIOM(dictVal.IsHolding<VtDictionary>());
    VtDictionary dict = dictVal.Get<VtDictionary>();
    TF_AXIOM(dict.GetValueAtPath("hello:world"));
    TF_AXIOM(*dict.GetValueAtPath("hello:world") == VtValue("value"));

    // Get the one value through Sdf API.
    TF_AXIOM(layer->GetFieldDictValueByKey(
                 fooPath, SdfFieldKeys->CustomData, TfToken("hello:world")) ==
             VtValue("value"));

    // Erase the key through the Sdf API.
    layer->EraseFieldDictValueByKey(
        fooPath, SdfFieldKeys->CustomData, TfToken("hello:world"));

    TF_AXIOM(layer->GetField(fooPath, SdfFieldKeys->CustomData).IsEmpty());
}

static void
_TestSdfLayerTimeSampleValueType()
{
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();
    SdfPrimSpecHandle foo = SdfCreatePrimInLayer(layer, SdfPath("/foo"));
    SdfAttributeSpecHandle attr =
        SdfAttributeSpec::New(foo, "attr", SdfValueTypeNames->Double);

    double value = 0.0;
    VtValue vtValue;

    // Set a double time sample into the double-valued attribute and
    // ensure that we get the same value back and that it maintains its
    // type.
    layer->SetTimeSample<double>(attr->GetPath(), 0.0, 1.0);
    TF_AXIOM(layer->QueryTimeSample(attr->GetPath(), 0.0, &value));
    TF_AXIOM(value == 1.0);
    TF_AXIOM(layer->QueryTimeSample(attr->GetPath(), 0.0, &vtValue));
    TF_AXIOM(vtValue.IsHolding<double>());
    TF_AXIOM(vtValue.UncheckedGet<double>() == 1.0);
    
    layer->SetTimeSample(attr->GetPath(), 1.0, VtValue(double(2.0)));
    TF_AXIOM(layer->QueryTimeSample(attr->GetPath(), 1.0, &value));
    TF_AXIOM(value == 2.0);
    TF_AXIOM(layer->QueryTimeSample(attr->GetPath(), 1.0, &vtValue));
    TF_AXIOM(vtValue.IsHolding<double>());
    TF_AXIOM(vtValue.UncheckedGet<double>() == 2.0);

    // Now try setting a float into the double-valued attribute.
    // The value should be converted to a double, and that's how
    // we should get it back.
    layer->SetTimeSample<float>(attr->GetPath(), 3.0, 3.0);
    TF_AXIOM(layer->QueryTimeSample(attr->GetPath(), 3.0, &value));
    TF_AXIOM(value == 3.0);
    TF_AXIOM(layer->QueryTimeSample(attr->GetPath(), 3.0, &vtValue));
    TF_AXIOM(vtValue.IsHolding<double>());
    TF_AXIOM(vtValue.UncheckedGet<double>() == 3.0);
    
    layer->SetTimeSample(attr->GetPath(), 4.0, VtValue(float(4.0)));
    TF_AXIOM(layer->QueryTimeSample(attr->GetPath(), 4.0, &value));
    TF_AXIOM(value == 4.0);
    TF_AXIOM(layer->QueryTimeSample(attr->GetPath(), 4.0, &vtValue));
    TF_AXIOM(vtValue.IsHolding<double>());
    TF_AXIOM(vtValue.UncheckedGet<double>() == 4.0);
}

static void
_TestSdfLayerTransferContents()
{
    // Test for bug where transferring an empty over (an inert spec)
    // from a layer would be registered as the addition of a non-inert 
    // spec.
    struct _ChangeListener : public TfWeakBase
    {
    public:
        _ChangeListener()
        {
            TfNotice::Register(
                TfCreateWeakPtr(this), &_ChangeListener::OnChangeNotice);
        }

        void OnChangeNotice(const SdfNotice::LayersDidChange& n)
        {
            changeListMap = n.GetChangeListMap();
        }

        SdfLayerChangeListMap changeListMap;
    };

    const SdfPath fooPath("/Foo");
    SdfLayerRefPtr srcLayer = SdfLayer::CreateAnonymous();
    SdfCreatePrimInLayer(srcLayer, fooPath);

    _ChangeListener l;
    SdfLayerRefPtr dstLayer = SdfLayer::CreateAnonymous();
    dstLayer->TransferContent(srcLayer);

    TF_AXIOM(l.changeListMap.count(dstLayer));
    TF_AXIOM(l.changeListMap[dstLayer].GetEntryList().count(fooPath));
    TF_AXIOM(l.changeListMap[dstLayer].GetEntryList()
             .find(fooPath)->second.flags.didAddInertPrim);
}

int
main(int argc, char **argv)
{
    _TestSdfLayerDictKeyOps();
    _TestSdfLayerTimeSampleValueType();
    _TestSdfLayerTransferContents();

    return 0;
}
