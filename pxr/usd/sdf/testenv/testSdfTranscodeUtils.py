#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.

from pxr import Sdf, Tf
import unittest

class TestTranscodeUtils(unittest.TestCase):

    # Encode tests

    def test_encode_empty(self):
        for format in [
            Sdf.TranscodeFormat.ASCII,
            Sdf.TranscodeFormat.UNICODE_XID
        ]:
            with self.subTest(format=format):
                self.assertEqual(
                    Sdf.EncodeIdentifier("", format),
                    "tn__",
                )

    def test_encode_invalid_utf8(self):
        for format in [
            Sdf.TranscodeFormat.ASCII,
            Sdf.TranscodeFormat.UNICODE_XID
        ]:
            for invalid in [
                b"\x83",
                b"\xc3\x28",
                b"\xe2\x82\x28",
                b"\xf0\x28\x8c\x28",
            ]:
                with self.subTest(format=format, invalid=invalid):
                    with self.assertRaises(Tf.ErrorException):
                        Sdf.EncodeIdentifier(invalid, format)

    def test_encode_complies_format(self):
        for identifier, format in [
            ("hello_world", Sdf.TranscodeFormat.ASCII),
            ("カーテンウォール", Sdf.TranscodeFormat.UNICODE_XID),
            ("tn__123456555_oDT", Sdf.TranscodeFormat.ASCII),
            ("tn__straße3_j7", Sdf.TranscodeFormat.UNICODE_XID),
        ]:
            with self.subTest(identifier=identifier, format=format):
                self.assertEqual(
                    Sdf.EncodeIdentifier(identifier, format),
                    identifier,
                )

    def test_encode(self):
        # Format independent
        for format in [
            Sdf.TranscodeFormat.ASCII,
            Sdf.TranscodeFormat.UNICODE_XID
        ]:
            for identifier, transcoded in [
                ("123-456/555", "tn__123456555_oDT"),
                ("#123 4", "tn__1234_d4I"),
                ("1234567890", "tn__1234567890_"),
                ("😁", "tn__nqd3")
            ]:
                with self.subTest(format=format, identifier=identifier):
                    self.assertEqual(
                        Sdf.EncodeIdentifier(identifier, format),
                        transcoded,
                    )

        # Format dependent
        for identifier, transcoded, format in [
            ("カーテンウォール", "tn__sxB76l2Y5o0X16", Sdf.TranscodeFormat.ASCII),
            ("straße 3", "tn__strae3_h6im0", Sdf.TranscodeFormat.ASCII),
            ("tn__strae3_h6im0", "tn__strae3_h6im0", Sdf.TranscodeFormat.ASCII),
            ("straße 3", "tn__straße3_j7", Sdf.TranscodeFormat.UNICODE_XID),
            ("tn__straße3_j7", "tn__straße3_j7", Sdf.TranscodeFormat.UNICODE_XID),
            ("tn__strae3_h6im0", "tn__strae3_h6im0", Sdf.TranscodeFormat.UNICODE_XID),
        ]:
            with self.subTest(identifier=identifier, format=format):
                self.assertEqual(
                    Sdf.EncodeIdentifier(identifier, format),
                    transcoded,
                )

    # Decode tests

    def test_decode_empty(self):
        self.assertEqual(
            Sdf.DecodeIdentifier(""),
            "",
        )

    def test_decode_no_prefix(self):
        for identifier in [
            "hello_world",
            "カーテンウォール",
        ]:
            with self.subTest(identifier=identifier):
                self.assertEqual(
                    Sdf.DecodeIdentifier(identifier),
                    identifier,
                )

    def test_decode_invalid_utf8(self):
        for invalid in [
            b"tn__\x83",
            b"tn__\xc3\x28",
            b"tn__\xe2\x82\x28",
            b"tn__\xf0\x28\x8c\x28",
        ]:
            with self.subTest(invalid=invalid):
                with self.assertRaises(Tf.ErrorException):
                    Sdf.DecodeIdentifier(invalid)

    def test_decode_cannot_decode(self):
        for invalid in [
            "tn__hi_zzzzzzzzzzzzzzzzzzzzzzzzzzzzz",
            "tn__#"
        ]:
            with self.subTest(invalid=invalid):
                with self.assertRaises(Tf.ErrorException):
                    Sdf.DecodeIdentifier(invalid)

    def test_decode(self):
        for identifier, transcoded in [
            ("123-456/555", "tn__123456555_oDT"),
            ("#123 4", "tn__1234_d4I"),
            ("1234567890", "tn__1234567890_"),
            ("カーテンウォール", "tn__sxB76l2Y5o0X16"),
            ("straße 3", "tn__strae3_h6im0"),
            ("straße 3", "tn__straße3_j7"),
        ]:
            with self.subTest(identifier=identifier):
                self.assertEqual(
                    Sdf.DecodeIdentifier(transcoded),
                    identifier,
                )


if __name__ == "__main__":
    unittest.main()
