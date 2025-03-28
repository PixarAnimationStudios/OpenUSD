# Unicode in USD {#Usd_Page_UTF_8}

## Overview {#Usd_UTF_8_Overview}

Text, unless otherwise noted, should be assumed to be UTF-8 encoded. It's 
erroneous to describe USDA as an "ASCII" file format, as strings, tokens, and 
asset valued fields have been expected to support UTF-8 for several releases. 
USD 24.03 extends UTF-8 support to path and metadata identifiers.

This document aims to help users and developers reason about how to best build 
and validate UTF-8 content and tooling for USD.

## UTF-8 Encoding {#Usd_UTF_8_Encoding}

UTF-8 is a variable length encoding that is backwards compatible with ASCII. 
Every ASCII character and string is byte-equivalent to its UTF-8 encoded 
character and string. Users should think of UTF-8 strings as bytes representing 
"code points" in the Unicode code charts. A single code point may be represented 
by 1, 2, 3, or 4 byte sequences.

### Replacement Code Point {#Usd_UTF_8_Encoding_Replacement}

Not every 1, 2, 3, or 4 byte sequence represents a valid UTF-8 code point. When 
a byte sequence is invalid and cannot be decoded, USD replaces the sequence with
�. Note that USD does not have to decode most strings that pass through it and 
should not be relied on for validation of content.

### Normalization {#Usd_UTF_8_Encoding_Normalization}

UTF-8 encoded strings may have sequences of code points that describe equivalent 
text rendered to the user. As an example, the second letter in München can be 
represented in UTF-8 by a single code point or two code points (u with an umlaut 
modifier). USD does not enforce or apply normalization forms internally. To 
USD, these two representations of München are distinct.

While USD does not enforce a normalization form, Unicode "Normalization Form C" 
(NFC) is preferred when creating new tokens and paths. The Python library 
unicodedata can be used to normalize strings. Strict validators may choose to 
warn users about strings (including tokens and paths) that are not NFC 
normalized. In the above example, the two code points version of München would 
be flagged by a validator checking for NFC normalization.

EdwardVII (where VII is three capital ASCII letters) and EdwardⅦ (where Ⅶ is a 
single UTF-8 code point) are distinct string values even under NFC 
normalization. When a user facing interface involves fuzzy matching a string, 
the Unicode documentation recommends Unicode "Normalization Form KC" (NFKC) 
normalization so a user does not have to be aware of specific encoding 
semantics. Strict validators may choose to warn users about siblings that have 
colliding NFKC normalization representations.

## Language Support {#Usd_UTF_8_Language_Support}

### C++ {#Usd_UTF_8_Language_Support_CPP}

USD assumes that all C++ string types (including tokens, scene paths, and 
asset paths) are UTF-8 encoded unless otherwise specified. Applications must 
ensure content is properly UTF-8 encoded before using USD APIs. The C++ standard 
library does not provide a Unicode library, but many string operations designed 
for single byte ASCII character strings in both the C++ standard library and Tf 
will work without modification. Developers should verify by reading the 
documentation and including UTF-8 content in test cases. Tf provides a minimal 
set of Unicode utilities primarily for its own internal usage and does not aim 
to be a fully featured Unicode support library.

### Python {#Usd_UTF_8_Language_Support_Python}

Strings as of Python 3.0 are natively Unicode (though not UTF-8 encoded). It 
provides string operations like `casefold` for case insensitive comparison and a 
library unicodedata for some transformations and queries. Utilities in Boost 
Python and Tf handle string conversion to and from UTF-8 at the USD 
C++/Python language boundary.

## Identifiers {#Usd_UTF_8_Identifiers}

Identifiers are used to name prims, properties, and metadata fields. The Unicode 
specification provides two classes of code points, XID_Start and XID_Continue 
to validate identifiers. USD extends the XID_Start class with `_` to define its 
default identifier set.

USD path identifiers should be validated with SdfPath::IsValidIdentifier and 
SdfPath::IsValidNamespacedIdentifier. TfIsValidIdentifier and 
TfMakeValidIdentifier should generally not be used to validate and produce prim 
or path identifiers.

## Operation Quick Reference {#Usd_UTF_8_Operation_Reference}

This table lists common string operations and how to reason about them within 
USD's UTF-8 support.

Operation              | Recommendation
---------------------- | --------------
Equivalence (==)       | Strings (and tokens, paths, and assets) are considered equivalent by USD if their byte (and therefore code point) representations are equivalent.
Deterministic ordering | Ordering a valid UTF-8 string by bytes should be equivalent to ordering by code point without decoding (if each byte is interpreted as an unsigned char).
Backwards Compatible Deterministic Ordering | USD has a legacy sorting algorithm (TfDictionaryLessThan) which orders alphanumeric characters case independently. Case independent ordering cannot be trivially extended to the full set of UTF-8 code points, so only non-ASCII code points are ordered by code point value.
Collating              | USD does not provide advanced string ordering operations often known as collating.
Casefolding            | There is no support for general casefolding of UTF-8 strings. Use TfStringToLowerAscii to fold all ASCII characters in a UTF-8 string. TfStringToLower, TfStringToUpper, and TfStringCapitialize should not be used on UTF-8 strings.
Regular expressions    | TfPatternMatcher does not currently offer case insensitive matching of UTF-8 strings.
Tokenizing             | Splitting UTF-8 strings around common ASCII symbols like `/` or `.` does not generally require any special consideration. Use TfUtf8CodePointIterator if trying to find and split around a multi-byte code point.
Concatenation          | Concatenation of two valid UTF-8 strings is still a valid UTF-8 string, though normalization may not be preserved. 
Length                 | In C++, a string's length is its number of bytes, not the number of code points. The number of code points can be computed by taking the distance between a TfUtf8CodePointView's begin and end. In Python, `len` will count code points.
Path identifier validation | Do not use TfIsValidIdentifier as it will reject UTF-8 characters. Use SdfPath::IsValidIdentifier, SdfPath::IsValidNamespacedIdentifier, SdfSchemaBase::IsValidVariantIdentifier, and SdfSchemaBase::IsValidVariantSelection.

## Encoding Quick Reference {#Usd_UTF_8_Encoding_Reference}

This table records the encoding representations and rules for USD content. 
Strict validators can use the best practices to warn users about 
non-conforming content.

| Type or Context          | Encoding and Restrictions | Best Practices |
| ------------------------ | ------------------------- | -------------- |
| string (sdf value type)  | UTF-8                     | |
| token (sdf value type)   | UTF-8                     | Prefer NFC normalized |
| asset (sdf value type)   | UTF-8 (Protocols determine lookup equivalence) | See URI and IRI Specifications |
| prim identifier          | UTF-8 (Xid character class + leading `_`) | Prefer NFC normalized |
| property identifier      | UTF-8 (Xid character class + leading `_`). Property identifiers may be namespaced with medial `:`. | Prefer NFC normalized |
| variant set identifier   | UTF-8 (Xid character class + leading `_`) | Prefer NFC normalized |
| variant selection identifier | UTF-8 (Xid character class, with leading "continue" code points including `_` and digits) | Prefer NFC normalized |
| metadata field identifier | UTF-8 (Xid character class + leading `_`) | Prefer NFC normalized |
| schema type name         | ASCII C++ Identifier (alphanumeric + `_` with no leading digits) | |
| schema property name     | ASCII C++ Identifier (alphanumeric + `_` with no leading digits) | |
| file format extension (Sdf) | UTF-8. Only ASCII characters are casefolded for equivalence / dispatch. | Prefer casefolded |
| resolver scheme (Ar) | URI specification. Starts with a single ASCII letter, followed by any ASCII alphanumeric, `-`, `+`, and `.`. Casefolded for equivalence and dispatch. | Prefer casefolded |

## Additional Resources {#Usd_UTF_8_Additional_Resources}

- [Unicode Identifiers in USD proposal](https://github.com/PixarAnimationStudios/OpenUSD-proposals/tree/main/proposals/tf_utf8_identifiers)
- [Unicode Standard v15.0 (PDF)](https://www.unicode.org/versions/Unicode15.0.0/ch03.pdf)
- [Unicode Identifiers and Syntax](https://www.unicode.org/reports/tr31/)
