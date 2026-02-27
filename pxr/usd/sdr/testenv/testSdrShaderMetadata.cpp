//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/shaderNodeMetadata.h"

PXR_NAMESPACE_USING_DIRECTIVE

void
TestNodeLabel()
{
    // Test the typical behavior for a token-valued metadata item
    SdrShaderNodeMetadata m;
    TF_VERIFY(!m.HasLabel());
    TF_VERIFY(!m.HasItem(SdrNodeMetadata->Label));
    m.SetItem(SdrNodeMetadata->Label, TfToken("foo"));
    TF_VERIFY(m.HasLabel());
    TF_VERIFY(m.HasItem(SdrNodeMetadata->Label));
    TF_VERIFY(m.GetLabel() == TfToken("foo"));
    TF_VERIFY(m.GetItemValueAs<TfToken>(SdrNodeMetadata->Label)
              == m.GetLabel());
    TF_VERIFY(m.GetItemValue(SdrNodeMetadata->Label)
              == VtValue(TfToken("foo")));
    m.SetItem(SdrNodeMetadata->Label, TfToken(""));
    TF_VERIFY(m.HasLabel());
    TF_VERIFY(m.HasItem(SdrNodeMetadata->Label));
    m.ClearLabel();
    TF_VERIFY(!m.HasLabel());

    // Test that ingestion carries over the label value
    VtDictionary d;
    d[SdrNodeMetadata->Label] = TfToken("");
    m = SdrShaderNodeMetadata(std::move(d));
    TF_VERIFY(m.HasLabel());
    TF_VERIFY(m.HasItem(SdrNodeMetadata->Label));
    TF_VERIFY(m.GetItemValue(SdrNodeMetadata->Label) == VtValue(TfToken("")));

    // Test that setting label's value to an empty VtValue clears the item
    m.SetItem(SdrNodeMetadata->Label, VtValue());
    TF_VERIFY(!m.HasLabel());
    TF_VERIFY(!m.HasItem(SdrNodeMetadata->Label));
}

void
TestNodeOpenPages()
{
    // Tests the typical behavior for a metadata item with a complex type
    SdrShaderNodeMetadata m;
    m.SetItem(SdrNodeMetadata->OpenPages,
              SdrTokenVec({TfToken("foo"), TfToken("bar")}));
    TF_VERIFY(m.HasOpenPages());
    TF_VERIFY(m.GetOpenPages().size() == 2);

    // Test clearing the item
    m.ClearItem(SdrNodeMetadata->OpenPages);
    TF_VERIFY(!m.HasOpenPages());
    TF_VERIFY(m.GetOpenPages().size() == 0);
}

void
TestSdrShaderNodeMetadata()
{
    TestNodeLabel();
    TestNodeOpenPages();
}

int main()
{
    TestSdrShaderNodeMetadata();
}
