//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/sdf/globPattern.h"

#include <cstring>
#include <string_view>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// --------------------------------------------------------------------
// Op-byte and flag constants
// --------------------------------------------------------------------

static constexpr uint8_t _OpEnd         = 0;  // end-of-bytecode
static constexpr uint8_t _OpAny         = 1;  // ?
static constexpr uint8_t _OpClass       = 2;  // [a-zA-Z]
static constexpr uint8_t _OpNClass      = 3;  // [!0-9]
static constexpr uint8_t _OpLiteralBase = 4;  // >= literal text; length
// Max bytes per literal-run element.  The op-byte encodes (chunk + base) and
// must stay <= 255, so chunk <= 255 - _OpLiteralBase = 251.  Literals longer
// than this length are just encoded as consecutive runs.
static constexpr uint8_t _MaxLiteralRun = 251;

// Bits set in Sdf_GlobPattern::_flags.  Bit 15 (_Compiled) is reserved for
// the validity sentinel declared in the header.
static constexpr uint16_t _HasPrefix       = 1u << 0;
static constexpr uint16_t _HasSuffix       = 1u << 1;
static constexpr uint16_t _ExactMatch      = 1u << 2;
// Set when the pattern is the shape "*<literal>*": no prefix, no suffix,
// exactly one middle segment that is a single literal-run.  Match short-
// circuits to a bare substring search.
static constexpr uint16_t _SingleLitMiddle = 1u << 3;

static constexpr uint16_t _NoLiteral = 0xFFFF;

// Per-segment header (uniform for prefix, suffix, middles).
// minBytesRemaining is only read for middle segments.  firstLiteralOffset is
// _NoLiteral if the segment contains no literal run.  codePointsBefore counts
// the codepoints preceding the first literal run.
struct _SegHeader {
    uint16_t minBytesRemaining;
    uint16_t firstLiteralOffset;
    uint16_t codePointsBefore;
};

// Character-class range payload: serialized as a tightly packed pair.
struct _Range {
    uint32_t lo;
    uint32_t hi;
};

static constexpr size_t _SegHeaderSize = sizeof(_SegHeader);

// --------------------------------------------------------------------
// Bytestream read/write helpers
// --------------------------------------------------------------------
//
// memcpy-based to sidestep alignment / strict-aliasing concerns; modern
// compilers reliably lower these to direct loads/stores at -O2.

template <class T>
static inline void
_WriteAdv(uint8_t *&dst, const T &val)
{
    memcpy(dst, &val, sizeof(T));
    dst += sizeof(T);
}

template <class T>
static inline T
_ReadAdv(const uint8_t *&src)
{
    T val;
    memcpy(&val, src, sizeof(T));
    src += sizeof(T);
    return val;
}

// Find the first occurrence of a literal byte run (sliced from the bytecode)
// within a haystack string.  Returns a pointer into hay, or nullptr if not
// found.
static inline const char *
_FindLiteral(const char *hay, size_t haylen,
             const uint8_t *needle, size_t needlen)
{
    std::string_view hv(hay, haylen);
    std::string_view nv(reinterpret_cast<const char *>(needle), needlen);
    size_t pos = hv.find(nv);
    return pos == std::string_view::npos ? nullptr : hv.data() + pos;
}

// --------------------------------------------------------------------
// UTF-8 helpers
// --------------------------------------------------------------------

static inline int
_Utf8CodePointLen(uint8_t lead)
{
    if (lead < 0x80) {
        return 1;
    }
    if (lead < 0xE0) {
        return 2;
    }
    if (lead < 0xF0) {
        return 3;
    }
    return 4;
}

static inline uint32_t
_DecodeUtf8(const char *s, int *lenOut)
{
    uint8_t lead = static_cast<uint8_t>(s[0]);
    if (lead < 0x80) {
        *lenOut = 1;
        return lead;
    }
    if (lead < 0xE0) {
        *lenOut = 2;
        return ((lead & 0x1F) << 6) |
               (static_cast<uint8_t>(s[1]) & 0x3F);
    }
    if (lead < 0xF0) {
        *lenOut = 3;
        return ((lead & 0x0F) << 12) |
               ((static_cast<uint8_t>(s[1]) & 0x3F) << 6) |
               (static_cast<uint8_t>(s[2]) & 0x3F);
    }
    *lenOut = 4;
    return ((lead & 0x07) << 18) |
           ((static_cast<uint8_t>(s[1]) & 0x3F) << 12) |
           ((static_cast<uint8_t>(s[2]) & 0x3F) << 6) |
           (static_cast<uint8_t>(s[3]) & 0x3F);
}

// Back up codePointCount codepoints from end.  Returns pointer to start of the
// backed-up region, or nullptr if insufficient codepoints.
static inline const char *
_BackUpCodePoints(const char *begin, const char *end, uint16_t codePointCount)
{
    const char *p = end;
    for (uint16_t i = 0; i < codePointCount; ++i) {
        if (p <= begin) {
            return nullptr;
        }
        --p;
        while (p > begin && (static_cast<uint8_t>(*p) & 0xC0) == 0x80) {
            --p;
        }
    }
    return p;
}

// --------------------------------------------------------------------
// Compile
// --------------------------------------------------------------------

namespace {

// Intermediate representation of a single element in a segment.
struct _Element {
    uint8_t op; // _OpAny, _OpClass, _OpNClass, or >= _OpLiteralBase
    std::vector<uint8_t> data;
};

struct _Segment {
    std::vector<_Element> elements;

    // Computed during serialization.
    uint16_t minBytes = 0;
    uint16_t codePointCount = 0;
};

// Compute the minimum byte cost of one element.
static uint16_t
_ElementMinBytes(const _Element &el)
{
    if (el.op == _OpAny   ||
        el.op == _OpClass ||
        el.op == _OpNClass) {
        return 1;
    }
    // Literal run: byte count is the data size.
    return static_cast<uint16_t>(el.data.size());
}

// Compute the codepoint count of one element.
static uint16_t
_ElementCodePointCount(const _Element &el)
{
    if (el.op == _OpAny   ||
        el.op == _OpClass ||
        el.op == _OpNClass) {
        return 1;
    }
    // Literal run: count codepoints in the UTF-8 bytes.
    uint16_t count = 0;
    size_t i = 0;
    while (i < el.data.size()) {
        i += _Utf8CodePointLen(el.data[i]);
        ++count;
    }
    return count;
}

// Serialized byte size of one element (op-byte + payload).
static size_t
_ElementByteSize(const _Element &el)
{
    if (el.op == _OpAny) {
        return 1;
    }
    if (el.op == _OpClass ||
        el.op == _OpNClass) {
        // op + numRanges + numRanges * sizeof(_Range)
        uint8_t numRanges = el.data.size() > 0 ? el.data[0] : 0;
        return 1 + 1 + numRanges * sizeof(_Range);
    }
    // Literal: op-byte + data bytes.
    return 1 + el.data.size();
}

// Parse a character class starting after the '['.  Returns the position past
// the closing ']', or nullptr on error.
static const char *
_ParseClass(const char *p, const char *end, _Element *out)
{
    bool negated = false;
    if (p < end && (*p == '!' || *p == '^')) {
        negated = true;
        ++p;
    }
    out->op = negated ? _OpNClass : _OpClass;

    std::vector<_Range> ranges;

    // ']' as first char after optional negation is a literal ']'.
    bool first = true;
    while (p < end) {
        if (*p == ']' && !first) {
            break;
        }
        first = false;

        int codePointBytes;
        uint32_t codePoint = _DecodeUtf8(p, &codePointBytes);
        p += codePointBytes;

        // Check for range: codePoint-codePoint
        if (p + 1 < end && *p == '-' && *(p + 1) != ']') {
            ++p; // skip '-'
            int hiBytes;
            uint32_t hi = _DecodeUtf8(p, &hiBytes);
            p += hiBytes;
            ranges.push_back({codePoint, hi});
        } else {
            ranges.push_back({codePoint, codePoint});
        }
    }

    if (p >= end) {
        return nullptr; // unclosed '['
    }
    ++p; // skip ']'

    // Serialize: numRanges byte, then ranges as packed pairs.
    out->data.resize(1 + ranges.size() * sizeof(_Range));
    out->data[0] = static_cast<uint8_t>(ranges.size());
    uint8_t *dst = out->data.data() + 1;
    for (const _Range &r : ranges) {
        _WriteAdv(dst, r);
    }
    return p;
}

// Flush accumulated literal bytes into segment as one or more literal-run
// elements (splitting at _MaxLiteralRun boundaries).
static void
_FlushLiteral(std::vector<uint8_t> &litBuf, _Segment &seg)
{
    size_t off = 0;
    while (off < litBuf.size()) {
        size_t chunk = litBuf.size() - off;
        if (chunk > _MaxLiteralRun) {
            chunk = _MaxLiteralRun;
        }
        _Element el;
        el.op = static_cast<uint8_t>(chunk + _OpLiteralBase);
        el.data.assign(litBuf.begin() + off, litBuf.begin() + off + chunk);
        seg.elements.push_back(std::move(el));
        off += chunk;
    }
    litBuf.clear();
}

} // anon namespace

Sdf_GlobPattern::Sdf_GlobPattern(Sdf_GlobPattern const &other)
    : _bufSize(other._bufSize)
    , _minBytes(other._minBytes)
    , _flags(other._flags)
    , _suffixCodePointCount(other._suffixCodePointCount)
{
    if (other._buf) {
        _buf.reset(new uint8_t[_bufSize]);
        std::memcpy(_buf.get(), other._buf.get(), _bufSize);
    }
}

Sdf_GlobPattern &
Sdf_GlobPattern::operator=(Sdf_GlobPattern const &other)
{
    if (this != &other) {
        *this = Sdf_GlobPattern(other);
    }
    return *this;
}

Sdf_GlobPattern
Sdf_GlobPattern::Compile(const char *pattern, size_t patLen)
{
    Sdf_GlobPattern result;

    const char *p = pattern;
    const char *end = pattern + patLen;

    // Parse into segments separated by '*'.
    std::vector<_Segment> segments;
    segments.emplace_back();
    std::vector<uint8_t> litBuf;

    while (p < end) {
        char ch = *p;

        if (ch == '*') {
            // Flush literal, close current segment, start new one.
            _FlushLiteral(litBuf, segments.back());
            // Collapse consecutive '*'s.
            while (p < end && *p == '*') {
                ++p;
            }
            segments.emplace_back();
            continue;
        }

        if (ch == '?') {
            _FlushLiteral(litBuf, segments.back());
            _Element el;
            el.op = _OpAny;
            segments.back().elements.push_back(std::move(el));
            ++p;
            continue;
        }

        if (ch == '[') {
            _FlushLiteral(litBuf, segments.back());
            _Element el;
            const char *after = _ParseClass(p + 1, end, &el);
            if (!after) {
                return result; // parse error, result stays invalid
            }
            segments.back().elements.push_back(std::move(el));
            p = after;
            continue;
        }

        if (ch == '\\') {
            ++p;
            if (p >= end) {
                return result; // trailing backslash, parse error
            }
            // Escaped char is literal.
            int codePointBytes;
            _DecodeUtf8(p, &codePointBytes);
            for (int i = 0; i < codePointBytes; ++i) {
                litBuf.push_back(static_cast<uint8_t>(p[i]));
            }
            p += codePointBytes;
            continue;
        }

        // Regular character - accumulate literal.
        int codePointBytes;
        _DecodeUtf8(p, &codePointBytes);
        for (int i = 0; i < codePointBytes; ++i) {
            litBuf.push_back(static_cast<uint8_t>(p[i]));
        }
        p += codePointBytes;
    }

    // Flush any trailing literal.
    _FlushLiteral(litBuf, segments.back());

    // Identify prefix, suffix, middles.
    // - If there was no '*', segments has exactly 1 entry (prefix only, exact
    // match).
    // - If pattern was just "*", segments has 2 empty entries.
    // - If pattern was "A*B*C", segments = [A, B, C]: prefix=A, suffix=C,
    // middles=[B].

    bool hasStar = (segments.size() > 1);

    // Special case: pattern is entirely '*' (or '**' etc) - match anything.
    if (hasStar) {
        bool allEmpty = true;
        for (auto &s : segments) {
            if (!s.elements.empty()) {
                allEmpty = false;
                break;
            }
        }
        if (allEmpty) {
            // Match anything.
            result._buf = nullptr;
            result._bufSize = 0;
            result._minBytes = 0;
            result._flags = _Compiled;
            result._suffixCodePointCount = 0;
            return result;
        }
    }

    // Determine prefix/suffix/middles.
    _Segment *prefix = nullptr;
    _Segment *suffix = nullptr;
    std::vector<_Segment *> middles;

    if (!hasStar) {
        // No star: the single segment is both prefix and suffix (exact match).
        // We'll encode it as just a prefix.
        prefix = &segments[0];
    } else {
        if (!segments.front().elements.empty()) {
            prefix = &segments.front();
        }
        if (!segments.back().elements.empty()) {
            suffix = &segments.back();
        }
        for (size_t i = 1; i + 1 < segments.size(); ++i) {
            if (!segments[i].elements.empty()) {
                middles.push_back(&segments[i]);
            }
        }
    }

    // Compute per-segment stats.
    auto computeStats = [](_Segment &seg) {
        seg.minBytes = 0;
        seg.codePointCount = 0;
        for (auto &el : seg.elements) {
            seg.minBytes += _ElementMinBytes(el);
            seg.codePointCount += _ElementCodePointCount(el);
        }
    };
    if (prefix) {
        computeStats(*prefix);
    }
    if (suffix) {
        computeStats(*suffix);
    }
    for (auto *mid : middles) {
        computeStats(*mid);
    }

    // Compute serialized buffer size.
    auto segBufSize = [](_Segment &seg) -> size_t {
        size_t sz = _SegHeaderSize;
        for (auto &el : seg.elements) {
            sz += _ElementByteSize(el);
        }
        sz += 1; // for _OpEnd
        return sz;
    };

    size_t totalBufSize = 0;
    if (prefix) {
        totalBufSize += segBufSize(*prefix);
    }
    if (suffix) {
        totalBufSize += segBufSize(*suffix);
    }
    for (auto *mid : middles) {
        totalBufSize += segBufSize(*mid);
    }

    // Allocate and serialize.
    result._buf = std::make_unique<uint8_t[]>(totalBufSize);
    result._bufSize = static_cast<uint16_t>(totalBufSize);
    uint8_t *dst = result._buf.get();

    // Serialize one segment to bytecode.  minBytesRemaining is passed in.
    auto serializeSegment = [&dst](
        _Segment &seg, uint16_t minBytesRemaining)
    {
        // Find first literal offset and count codepoints preceding it.
        uint16_t firstLitOff = _NoLiteral;
        uint16_t codePointsBefore = 0;
        uint16_t byteOff = 0;
        for (auto &el : seg.elements) {
            if (el.op >= _OpLiteralBase) {
                firstLitOff = byteOff;
                break;
            }
            // _OpAny / _OpClass / _OpNClass each match exactly one codePoint.
            ++codePointsBefore;
            byteOff += static_cast<uint16_t>(_ElementByteSize(el));
        }

        _SegHeader hdr{minBytesRemaining, firstLitOff, codePointsBefore};
        _WriteAdv(dst, hdr);

        // Elements: op-byte, then payload (empty for _OpAny).
        for (auto &el : seg.elements) {
            *dst++ = el.op;
            if (!el.data.empty()) {
                memcpy(dst, el.data.data(), el.data.size());
                dst += el.data.size();
            }
        }
        *dst++ = _OpEnd;
    };

    // Compute minBytesRemaining suffix-sums for middles.
    std::vector<uint16_t> middleMinBytes(middles.size());
    {
        uint16_t sum = 0;
        for (size_t i = middles.size(); i > 0; --i) {
            sum += middles[i-1]->minBytes;
            middleMinBytes[i-1] = sum;
        }
    }

    // Serialize in buffer order: prefix, suffix, middles.  Only middles read
    // minBytesRemaining at match time; prefix and suffix headers are skipped,
    // so we pass 0 for them.
    if (prefix) {
        serializeSegment(*prefix, 0);
    }
    if (suffix) {
        serializeSegment(*suffix, 0);
    }
    for (size_t i = 0; i < middles.size(); ++i) {
        serializeSegment(*middles[i], middleMinBytes[i]);
    }

    // Fill struct fields.
    result._minBytes = 0;
    if (prefix) {
        result._minBytes += prefix->minBytes;
    }
    if (suffix) {
        result._minBytes += suffix->minBytes;
    }
    if (!middleMinBytes.empty()) {
        result._minBytes += middleMinBytes[0];
    }

    uint16_t flags = _Compiled;
    if (prefix) {
        flags |= _HasPrefix;
    }
    if (suffix) {
        flags |= _HasSuffix;
    }

    // Suffix codePoint count: 0 means "single literal run" fast path.
    uint16_t suffixCodePointCount = 0;
    if (suffix) {
        // Check if suffix is a single literal run element.
        if (suffix->elements.size() == 1 &&
            suffix->elements[0].op >= _OpLiteralBase) {
            suffixCodePointCount = 0;
        } else {
            suffixCodePointCount = suffix->codePointCount;
        }
    }

    // Detect the common "*<literal>*" case: no prefix, no suffix, and a single
    // middle segment that is a single literal-run.  Match() can then skip
    // straight to a bare substring search.
    if (!prefix && !suffix && middles.size() == 1 &&
        middles[0]->elements.size() == 1 &&
        middles[0]->elements[0].op >= _OpLiteralBase) {
        flags |= _SingleLitMiddle;
    }

    // No-star patterns must match the entire string; flag for Match() to verify
    // nothing is left over after the prefix.
    if (!hasStar) {
        flags |= _ExactMatch;
    }
    result._flags = flags;
    result._suffixCodePointCount = suffixCodePointCount;

    return result;
}

// --------------------------------------------------------------------
// Match
// --------------------------------------------------------------------

// Attempt to match the element stream at *pp against the string starting at
// str.  Returns the pointer past the matched bytes in str on success, or
// nullptr on failure.  Advances *pp past the _OpEnd byte.
static const char *
_MatchAnchored(const uint8_t **pp, const char *str, const char *strEnd)
{
    const uint8_t *p = *pp;
    while (*p != _OpEnd) {
        uint8_t op = *p++;
        if (op == _OpAny) {
            if (str >= strEnd) {
                return nullptr;
            }
            str += _Utf8CodePointLen(static_cast<uint8_t>(*str));
        } else if (op == _OpClass || op == _OpNClass) {
            if (str >= strEnd) {
                return nullptr;
            }
            int codePointBytes;
            uint32_t codePoint = _DecodeUtf8(str, &codePointBytes);
            uint8_t numRanges = *p++;
            bool inClass = false;
            for (uint8_t i = 0; i < numRanges; ++i) {
                _Range r = _ReadAdv<_Range>(p);
                if (codePoint >= r.lo && codePoint <= r.hi) {
                    inClass = true;
                }
            }
            bool want = (op == _OpClass);
            if (inClass != want) {
                return nullptr;
            }
            str += codePointBytes;
        } else {
            // Literal run.
            uint16_t runLen = op - _OpLiteralBase;
            if (str + runLen > strEnd) {
                return nullptr;
            }
            if (memcmp(str, p, runLen) != 0) {
                return nullptr;
            }
            p += runLen;
            str += runLen;
        }
    }
    ++p; // skip _OpEnd
    *pp = p;
    return str;
}

// Compute the byte size of a segment's element stream (excluding _OpEnd).
// Used to skip past a segment without matching it.
static const uint8_t *
_SkipSegment(const uint8_t *p)
{
    while (*p != _OpEnd) {
        uint8_t op = *p++;
        if (op == _OpAny) {
            // no payload
        } else if (op == _OpClass || op == _OpNClass) {
            uint8_t numRanges = *p++;
            p += numRanges * sizeof(_Range);
        } else {
            uint16_t runLen = op - _OpLiteralBase;
            p += runLen;
        }
    }
    return p + 1; // skip _OpEnd
}

bool
Sdf_GlobPattern::Match(const char *str, size_t len) const
{
    // Invalid pattern never matches.
    if (!*this) {
        return false;
    }

    // Early-out on minimum length.
    if (len < _minBytes) {
        return false;
    }

    // Empty buf: either match-anything (star-only) or match empty string.
    if (!_buf) {
        return true; // compiled from "*" or similar
    }

    uint16_t flags = _flags;

    // Fast path: "*<literal>*".  Just a substring search.
    if (flags & _SingleLitMiddle) {
        const uint8_t *litOp = _buf.get() + _SegHeaderSize;
        uint16_t litLen = *litOp - _OpLiteralBase;
        const uint8_t *litData = litOp + 1;
        return _FindLiteral(str, len, litData, litLen) != nullptr;
    }

    const char    *strBegin = str;
    const char    *strEnd   = str + len;
    const uint8_t *p        = _buf.get();
    const uint8_t *bufEnd   = _buf.get() + _bufSize;

    bool hasPrefix  = (flags & _HasPrefix);
    bool hasSuffix  = (flags & _HasSuffix);
    bool exactMatch = (flags & _ExactMatch);
    uint16_t suffixCodePointCount = _suffixCodePointCount;

    // Match prefix.
    if (hasPrefix) {
        p += _SegHeaderSize; // skip header
        const char *after = _MatchAnchored(&p, strBegin, strEnd);
        if (!after) {
            return false;
        }
        strBegin = after;
    }

    // Match suffix.
    if (hasSuffix) {
        p += _SegHeaderSize; // skip header

        if (suffixCodePointCount == 0) {
            // Single literal run: fast path.
            uint8_t op = *p;
            uint16_t byteLen = op - _OpLiteralBase;
            if ((strEnd - strBegin < byteLen) ||
                (memcmp(strEnd - byteLen, p + 1, byteLen) != 0)) {
                return false;
            }
            strEnd -= byteLen;
            p = _SkipSegment(p);
        } else {
            // Back up suffixCodePointCount codepoints from end.
            const char *suffixStart =
                _BackUpCodePoints(strBegin, strEnd, suffixCodePointCount);
            if (!suffixStart) {
                return false;
            }
            const uint8_t *pCopy = p;
            const char *after = _MatchAnchored(&pCopy, suffixStart, strEnd);
            if (!after || after != strEnd) {
                return false;
            }
            strEnd = suffixStart;
            p = pCopy;
        }
    }

    // Exact match check (no-star patterns).
    if (exactMatch) {
        return strBegin == strEnd;
    }

    // Match middles.
    while (p < bufEnd) {
        _SegHeader hdr = _ReadAdv<_SegHeader>(p);

        size_t remaining = strEnd - strBegin;
        if (remaining < hdr.minBytesRemaining) {
            return false;
        }

        // Search for this segment in [strBegin, strEnd).
        bool found = false;

        if (hdr.firstLiteralOffset != _NoLiteral) {
            // Substring-search acceleration: find the literal run, then verify
            // full segment around it.
            const uint8_t *litOp   = p + hdr.firstLiteralOffset;
            uint16_t       litLen  = *litOp - _OpLiteralBase;
            const uint8_t *litData = litOp + 1;

            // Tighter path when the literal IS the entire segment: skip the
            // anchored-match dance (it would only redo a memcmp the substring
            // search already proved).  Detected when there are no preceding
            // ops and the byte after the literal is _OpEnd.
            if (hdr.firstLiteralOffset == 0 && litData[litLen] == _OpEnd) {
                const char *hit =
                    _FindLiteral(strBegin, strEnd - strBegin, litData, litLen);
                if (!hit) {
                    return false;
                }
                strBegin = hit + litLen;
                p = litData + litLen + 1; // past _OpEnd
                continue;
            }

            const char *searchPos = strBegin;
            while (true) {
                size_t searchLen = strEnd - searchPos;
                if (searchLen < litLen) {
                    break;
                }

                // Find the literal in the remaining string.
                const char *hit =
                    _FindLiteral(searchPos, searchLen, litData, litLen);
                if (!hit) {
                    break;
                }

                // Back up codePointsBefore codepoints from hit.
                const char *segStartStr = hit;
                if (hdr.codePointsBefore > 0) {
                    segStartStr =
                        _BackUpCodePoints(strBegin, hit, hdr.codePointsBefore);
                    if (!segStartStr) {
                        searchPos = hit + 1;
                        continue;
                    }
                }

                // Try anchored match from segStartStr.
                const uint8_t *pTry = p;
                const char *after = _MatchAnchored(&pTry, segStartStr, strEnd);
                if (after) {
                    strBegin = after;
                    p = pTry;
                    found = true;
                    break;
                }
                searchPos = hit + 1;
            }
        } else {
            // No literal in segment (all ?/classes).  Sliding window.
            const char *pos = strBegin;
            while (pos <= strEnd) {
                const uint8_t *pTry = p;
                const char *after = _MatchAnchored(&pTry, pos, strEnd);
                if (after) {
                    strBegin = after;
                    p = pTry;
                    found = true;
                    break;
                }
                if (pos >= strEnd) {
                    break;
                }
                pos += _Utf8CodePointLen(static_cast<uint8_t>(*pos));
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
