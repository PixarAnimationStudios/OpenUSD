#include "pxr/pxr.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/usd/sdf/transcodeUtils.h"

PXR_NAMESPACE_USING_DIRECTIVE

static void
TestEncodeEmpty()
{
    TF_AXIOM(SdfEncodeIdentifier("", SdfTranscodeFormat::ASCII) == "tn__");
    TF_AXIOM(SdfEncodeIdentifier("", SdfTranscodeFormat::UNICODE_XID) == "tn__");
}

static void
TestEncodeCompliesFormat()
{
    TF_AXIOM(SdfEncodeIdentifier("hello_world", SdfTranscodeFormat::ASCII) == "hello_world");
    TF_AXIOM(SdfEncodeIdentifier("カーテンウォール", SdfTranscodeFormat::UNICODE_XID) == "カーテンウォール");
    TF_AXIOM(SdfEncodeIdentifier("tn__123456555_oDT", SdfTranscodeFormat::ASCII) == "tn__123456555_oDT");
    TF_AXIOM(SdfEncodeIdentifier("tn__straße3_j7", SdfTranscodeFormat::UNICODE_XID) == "tn__straße3_j7");
}

static void
TestEncode()
{
    TF_AXIOM(SdfEncodeIdentifier("123-456/555", SdfTranscodeFormat::ASCII) == "tn__123456555_oDT");
    TF_AXIOM(SdfEncodeIdentifier("123-456/555", SdfTranscodeFormat::UNICODE_XID) == "tn__123456555_oDT");
    TF_AXIOM(SdfEncodeIdentifier("#123 4", SdfTranscodeFormat::ASCII) == "tn__1234_d4I");
    TF_AXIOM(SdfEncodeIdentifier("#123 4", SdfTranscodeFormat::UNICODE_XID) == "tn__1234_d4I");
    TF_AXIOM(SdfEncodeIdentifier("1234567890", SdfTranscodeFormat::ASCII) == "tn__1234567890_");
    TF_AXIOM(SdfEncodeIdentifier("1234567890", SdfTranscodeFormat::UNICODE_XID) == "tn__1234567890_");
    TF_AXIOM(SdfEncodeIdentifier("😁", SdfTranscodeFormat::ASCII) == "tn__nqd3");
    TF_AXIOM(SdfEncodeIdentifier("😁", SdfTranscodeFormat::UNICODE_XID) == "tn__nqd3");

    TF_AXIOM(SdfEncodeIdentifier("カーテンウォール", SdfTranscodeFormat::ASCII) == "tn__sxB76l2Y5o0X16");
    TF_AXIOM(SdfEncodeIdentifier("straße 3", SdfTranscodeFormat::ASCII) == "tn__strae3_h6im0");
    TF_AXIOM(SdfEncodeIdentifier("tn__strae3_h6im0", SdfTranscodeFormat::ASCII) == "tn__strae3_h6im0");
    TF_AXIOM(SdfEncodeIdentifier("straße 3", SdfTranscodeFormat::UNICODE_XID) == "tn__straße3_j7");
    TF_AXIOM(SdfEncodeIdentifier("tn__straße3_j7", SdfTranscodeFormat::UNICODE_XID) == "tn__straße3_j7");
    TF_AXIOM(SdfEncodeIdentifier("tn__strae3_h6im0", SdfTranscodeFormat::UNICODE_XID) == "tn__strae3_h6im0");
}

static void
TestDecodeEmpty()
{
    TF_AXIOM(SdfDecodeIdentifier("tn__") == "");
}

static void
TestDecodeNoPrefix()
{
    TF_AXIOM(SdfDecodeIdentifier("hello_world") == "hello_world");
    TF_AXIOM(SdfDecodeIdentifier("カーテンウォール") == "カーテンウォール");
}

static void
TestDecode()
{
    TF_AXIOM(SdfDecodeIdentifier("tn__123456555_oDT") == "123-456/555");
    TF_AXIOM(SdfDecodeIdentifier("tn__1234_d4I") == "#123 4");
    TF_AXIOM(SdfDecodeIdentifier("tn__1234567890_") == "1234567890");
    TF_AXIOM(SdfDecodeIdentifier("tn__sxB76l2Y5o0X16") == "カーテンウォール");
    TF_AXIOM(SdfDecodeIdentifier("tn__strae3_h6im0") == "straße 3");
    TF_AXIOM(SdfDecodeIdentifier("tn__straße3_j7") == "straße 3");
}

int
main(int argc, char **argv)
{
    TestEncodeEmpty();
    TestEncodeCompliesFormat();
    TestEncode();

    TestDecodeEmpty();
    TestDecodeNoPrefix();
    TestDecode();

    printf(">>> Test SUCCEEDED\n");
    return 0;
}

