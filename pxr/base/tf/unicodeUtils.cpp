//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/unicodeUtils.h"

#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <deque>
#include <tuple>
#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

namespace TfUnicodeUtils {
    ///
    /// Appends the UTF-8 byte representation of \c codePoint to \c result.
    /// 
    void AppendUTF8Char(uint32_t codePoint, std::string& result)
    {
        if (codePoint < 0x80)
        {
            // 1-byte UTF-8 encoding
            result.push_back(static_cast<char>(static_cast<unsigned char>(codePoint)));
        }
        else if (codePoint < 0x800)
        {
            // 2-byte UTF-8 encoding
            result.push_back(static_cast<char>(static_cast<unsigned char>((codePoint >> 6) | 0xc0)));
            result.push_back(static_cast<char>(static_cast<unsigned char>((codePoint & 0x3f) | 0x80)));
        }
        else if (codePoint < 0x10000)
        {
            // 3-byte UTF-8 encoding
            result.push_back(static_cast<char>(static_cast<unsigned char>((codePoint >> 12) | 0xe0)));
            result.push_back(static_cast<char>(static_cast<unsigned char>(((codePoint >> 6) & 0x3f) | 0x80)));
            result.push_back(static_cast<char>(static_cast<unsigned char>((codePoint & 0x3f) | 0x80)));
        }
        else
        {
            // 4-byte UTF-8 encoding
            result.push_back(static_cast<char>(static_cast<unsigned char>((codePoint >> 18) | 0xf0)));
            result.push_back(static_cast<char>(static_cast<unsigned char>(((codePoint >> 12) & 0x3f) | 0x80)));
            result.push_back(static_cast<char>(static_cast<unsigned char>(((codePoint >> 6) & 0x3f) | 0x80)));
            result.push_back(static_cast<char>(static_cast<unsigned char>((codePoint & 0x3f) | 0x80)));
        }
    }

    std::string MakeValidUTF8Identifier(const std::string& identifier)
    {
        std::string result;

        // empty strings are always associated with the '_' identifier
        if (identifier.empty())
        {
            result.push_back('_');
            return result;
        }

        // maximum size is always the number of bytes in the UTF-8 encoded string
        // but if a character is invalid it will be replaced by a '_' character, which
        // may compress a e.g., 4-byte UTF-8 invalid character into a single valid 1-byte UTF-8 '_' character
        result.reserve(identifier.size());
        utf8_const_iterator iterator(identifier.begin(), identifier.end());

        // first UTF-8 character must be in XID_Start
        if (!IsUTF8CharXIDStart(identifier, iterator.Wrapped()))
        {
            result.push_back('_');
        }
        else
        {
            AppendUTF8Char(*iterator, result);
        }

        for (++iterator; iterator != identifier.end(); iterator++)
        {
            if (!IsUTF8CharXIDContinue(identifier, iterator.Wrapped()))
            {
                result.push_back('_');
            }
            else
            {
                AppendUTF8Char(*iterator, result);
            }
        }

        return result;
    }

    std::string MakeValidUTF8PrimName(const std::string& primName)
    {
        std::string result;

        // empty strings are always associated with the '_' identifier
        if (primName.empty())
        {
            result.push_back('_');
            return result;
        }

        // maximum size is always the number of bytes in the UTF-8 encoded string
        // but if a character is invalid it will be replaced by a '_' character, which
        // may compress a e.g., 4-byte UTF-8 invalid character into a single valid 1-byte UTF-8 '_' character
        result.reserve(primName.size());
        utf8_const_iterator iterator(primName.begin(), primName.end());

        // for prim names the rule is all characters must be in XID_Start / XID_Continue
        // (XID_Start is captured in IsUTF8CharXIDContinue to avoid 2 function calls)
        for (; iterator != primName.end(); iterator++)
        {
            if (!IsUTF8CharXIDContinue(primName, iterator.Wrapped()))
            {
                result.push_back('_');
            }
            else
            {
                AppendUTF8Char(*iterator, result);
            }
        }

        return result;
    }

    std::string UTF8StringToLower(const std::string& source)
    {
        // case mapping may increase the length
        // of the string, but in most cases this probably
        // doesn't happen, so we can preallocate the size
        // according to the length of the original string
        std::string result;
        size_t length = source.length();
        result.reserve(length);

        utf8_const_iterator utf8Iterator(source.begin(), source.end());
        for (; utf8Iterator != source.end(); utf8Iterator++)
        {
            uint32_t codePoint = *utf8Iterator;
            if (unicodeCaseMapUpperToLower.find(codePoint) != unicodeCaseMapUpperToLower.end())
            {
                // this has a target to map to
                AppendUTF8Char(unicodeCaseMapUpperToLower[codePoint], result);
                
            }
            else if (unicodeCaseMapUpperToMultiLower.find(codePoint) != unicodeCaseMapUpperToMultiLower.end())
            {
                // this is an expansion
                std::vector<uint32_t> mappedLower = unicodeCaseMapUpperToMultiLower[codePoint];
                for (size_t i = 0; i < mappedLower.size(); i++)
                {
                    AppendUTF8Char(mappedLower[i], result);
                }
            }
            else
            {
                // when no target is present, we treat it as a simple case map (class 'C')
                // with a target of itself - we only have the codepoint here, so we need
                // to query our iterator for the raw bytes at the current position
                AppendUTF8Char(codePoint, result);
            }
        }

        return result;
    }

    std::string UTF8StringToUpper(const std::string& source)
    {
        // case mapping is a little tougher in this direction
        // because multiple bytes may be required to go back to the
        // lower case code point
        std::string result;
        size_t length = source.length();
        result.reserve(length);

        utf8_const_iterator utf8Iterator(source.begin(), source.end());
        for (; utf8Iterator != source.end(); utf8Iterator++)
        {
            uint32_t codePoint = *utf8Iterator;
            if (unicodeCaseMapLowerToUpper.find(codePoint) != unicodeCaseMapLowerToUpper.end())
            {
                // this is a single code point to single code point mapping
                AppendUTF8Char(unicodeCaseMapLowerToUpper[codePoint], result);
            }
            else if (unicodeCaseMapLowerToMultiUpper.find(codePoint) != unicodeCaseMapLowerToMultiUpper.end())
            {
                // single to multiple code point mapping
                std::vector<uint32_t> mappedUpper = unicodeCaseMapLowerToMultiUpper[codePoint];
                for (size_t i = 0; i < mappedUpper.size(); i++)
                {
                    AppendUTF8Char(mappedUpper[i], result);
                }
            }
            else
            {
                // we treat this as a simple case map (class 'C') with a target of itself
                AppendUTF8Char(codePoint, result);
            }
        }

        return result;
    }

    std::string UTF8StringCapitalize(const std::string& source)
    {
        // same rules apply on length as going lower -> upper
        std::string result;
        size_t length = source.length();
        result.reserve(length);

        utf8_const_iterator utf8Iterator(source.begin(), source.end());

        // we only want the first UTF8 char to have special treatment
        uint32_t codePoint = *utf8Iterator;
        if (unicodeCaseMapLowerToUpper.find(codePoint) != unicodeCaseMapLowerToUpper.end())
        {
            AppendUTF8Char(unicodeCaseMapLowerToUpper[codePoint], result);
        }
        else if (unicodeCaseMapLowerToMultiUpper.find(codePoint) != unicodeCaseMapLowerToMultiUpper.end())
        {
            // single to multiple code point mapping
            std::vector<uint32_t> mappedUpper = unicodeCaseMapLowerToMultiUpper[codePoint];
            for (size_t i = 0; i < mappedUpper.size(); i++)
            {
                AppendUTF8Char(mappedUpper[i], result);
            }
        }
        else
        {
            // when no target is present, we treat it as a simple case map (class 'C')
            // with a target of itself - we only have the codepoint here, so we need
            // to query our iterator for the raw bytes at the current position
            AppendUTF8Char(codePoint, result);
        }

        // now just append the rest
        utf8Iterator++;
        for (; utf8Iterator != source.end(); utf8Iterator++)
        {
            AppendUTF8Char(*utf8Iterator, result);
        }

        return result;
    }

    ///
    /// Computes a single hash value from a set of individual code points. 
    /// 
    /// This algorithm is taken from https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector/72073933#72073933
    /// 
    uint32_t ComputeHash(const std::vector<uint32_t>& codePoints)
    {
        uint32_t hash = codePoints.size();
        for (auto codePoint : codePoints)
        {
            codePoint = ((codePoint >> 16) ^ codePoint) * 0x45d9f3b;
            codePoint = ((codePoint >> 16) ^ codePoint) * 0x45d9f3b;
            codePoint = (codePoint >> 16) ^ codePoint;
            hash ^= codePoint + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }

        return hash;
    }

    ///
    /// Normalizes \c source to NFD by recursively decomposing the 
    /// UTF8 characters to their decomposition mapping equivalents.
    ///
    /// Returns a vector of code points represented the normalized string sequence.
    /// 
    std::vector<uint32_t> Normalize(const std::string& source)
    {
        std::vector<uint32_t> result;
        
        // normalization may inflate / deflate source
        // but on average the original length of the string
        // is probably more than enough for the number
        // of code points that will be generated given
        // multi-byte reps of the characters in source
        size_t length = source.length();
        result.reserve(length);

        utf8_const_iterator utf8Iterator(source.begin(), source.end());
        for (; utf8Iterator != source.end(); utf8Iterator++)
        {
            uint32_t codePoint = *utf8Iterator;
            if (unicodeDecompositionMapping.find(codePoint) != unicodeDecompositionMapping.end())
            {
                // this character has a decomposition mapping
                // need to recursively decompose it's mappings
                // push the first decomposition onto the deque
                // then process until empty
                std::deque<uint32_t> codePoints;
                for (size_t i = 0; i < unicodeDecompositionMapping[codePoint].size(); i++)
                {
                    codePoints.push_back(unicodeDecompositionMapping[codePoint][i]);
                }

                while (!codePoints.empty())
                {
                    // take the code point off the front of the deque
                    // and see if it can be decomposed
                    uint32_t decomposedCodePoint = codePoints.front();
                    codePoints.pop_front();
                    if (unicodeDecompositionMapping.find(decomposedCodePoint) != unicodeDecompositionMapping.end())
                    {
                        // this itself has a decomposition, and we need to add the decomp to the
                        // front of the deque in reverse order to maintain the original character ordering
                        for (size_t i = 0; i < unicodeDecompositionMapping[decomposedCodePoint].size(); i++)
                        {
                            codePoints.push_front(unicodeDecompositionMapping[decomposedCodePoint][unicodeDecompositionMapping[decomposedCodePoint].size() - i - 1]);
                        }
                    }
                    else
                    {
                        result.push_back(decomposedCodePoint);
                    }
                }
            }
            else if (codePoint >= 0xAC00 && codePoint <= 0xD7A3)
            {
                // Hangul needs to be algorithmically decomposed
                size_t lCount = 19;
                size_t vCount = 21;
                size_t tCount = 28;
                size_t nCount = vCount * tCount;
                size_t sCount = lCount * nCount;
                uint32_t sIndex = codePoint - 0xAC00;
                if (sIndex >= sCount)
                {
                    result.push_back(codePoint);
                }
                else
                {
                    uint32_t L = 0x1100 + static_cast<uint32_t>(sIndex / nCount);
                    uint32_t V = 0x1161 + static_cast<uint32_t>((sIndex % nCount) / tCount);
                    uint32_t T = 0x11A7 + static_cast<uint32_t>(sIndex % tCount);
                    result.push_back(L);
                    result.push_back(V);
                    if (T != 0x11A7)
                    {
                        result.push_back(T);
                    }
                }
            }
            else
            {
                // no decomposition mapping, can use directly
                result.push_back(codePoint);
            }
        }

        // now that the code points have been decomposed, we have to run the canonical ordering algorithm
        // on them, which guarantees that combining marks are in ascending order
        // this amounts to finding sub-sequences of non-starters (i.e. those with a non-zero value
        // for their ordering) and ordering them according to their value
        std::vector<uint32_t>::iterator subSequenceBeginPosition = result.end();
        for (std::vector<uint32_t>::iterator sortingIterator = result.begin(); sortingIterator != result.end(); sortingIterator++)
        {
            // check if the code point is a non-starter (i.e. it's cannoical combining class != 0)
            if (unicodeCanonicalCombiningClass.find(*sortingIterator) != unicodeCanonicalCombiningClass.end())
            {
                if (subSequenceBeginPosition == result.end())
                {
                    // the code point is a non-starter and we haven't set the begin position yet
                    // of the sub-sequence, so do that now
                    subSequenceBeginPosition = sortingIterator;
                }
            }
            else
            {
                // it's a starter, so we now end the sequence
                if (subSequenceBeginPosition != result.end())
                {
                    // we now need to sort the sequence low to high
                    // against the value of the canonical combining class
                    std::sort(subSequenceBeginPosition, sortingIterator,
                        [](const auto& lhs, const auto& rhs) {
                            return unicodeCanonicalCombiningClass[lhs] < unicodeCanonicalCombiningClass[rhs];
                        });

                    // reset the subsequence beginning
                    subSequenceBeginPosition = result.end();
                }
            }
        }

        // we have an additional case here where we may have encountered a sub-sequence start
        // but reached the end of the string before we could sort the end of the sequence
        if (subSequenceBeginPosition != result.end())
        {
            // we now need to sort the sequence low to high
            // against the value of the canonical combining class
            std::sort(subSequenceBeginPosition, result.end(),
                [](const auto& lhs, const auto& rhs) {
                    return unicodeCanonicalCombiningClass[lhs] < unicodeCanonicalCombiningClass[rhs];
                });
        }

        return result;
    }

    /// 
    /// Derives a collation element set from a given \c unmatchedCodePoint that does not exist in the DUCET maps.
    /// 
    /// Returns a vector of generated collation elements for the unmatched code point.
    /// 
    std::vector<uint64_t> DeriveCollationElement(uint32_t unmatchedCodePoint)
    {
        // we need to synthesize a collation element, for this we need to see if the code point
        // falls within a particular range - these are stored as tuples defining ranges
        // for the blocks we are interested in here
        uint16_t aaaa = 0;
        uint16_t bbbb = 0;
        if ((unmatchedCodePoint >= tangutCodePoints.first && unmatchedCodePoint <= tangutCodePoints.second) ||
            (unmatchedCodePoint >= tangutComponentCodePoints.first && unmatchedCodePoint <= tangutComponentCodePoints.second) ||
            (unmatchedCodePoint >= tangutSupplementCodePoints.first && unmatchedCodePoint <= tangutSupplementCodePoints.second))
        {
            // siniform ideographic scripts (tangut subtype)
            aaaa = 0xfb00;
            bbbb = (unmatchedCodePoint - 0x17000) | 0x8000;
        }
        else if (unmatchedCodePoint >= nushuCodePoints.first && unmatchedCodePoint <= nushuCodePoints.second)
        {
            // siniform ideographic scripts (nushu subtype)
            aaaa = 0xfb01;
            bbbb = (unmatchedCodePoint - 0x1b170) | 0x8000;
        }
        else if (unmatchedCodePoint >= khitanSmallScriptCodePoints.first && unmatchedCodePoint <= khitanSmallScriptCodePoints.second)
        {
            // siniform ideographic scripts (khitan small script subtype)
            aaaa = 0xfb02;
            bbbb = (unmatchedCodePoint - 0x18b00) | 0x8000;
        }
        else if ((unmatchedCodePoint >= cjkUnifiedIdeographsCodePoints.first && unmatchedCodePoint <= cjkUnifiedIdeographsCodePoints.second) ||
            (unmatchedCodePoint >= cjkCompatibilityIdeographsCodePoints.first && unmatchedCodePoint <= cjkCompatibilityIdeographsCodePoints.second))
        {
            // han (core han unified ideographs subtype)
            aaaa = 0xfb40 + (unmatchedCodePoint >> 15);
            bbbb = (unmatchedCodePoint & 0x7fff) | 0x8000;
        }
        else if ((unmatchedCodePoint >= cjkUnifiedIdeographsACodePoints.first && unmatchedCodePoint <= cjkUnifiedIdeographsACodePoints.second) ||
            (unmatchedCodePoint >= cjkUnifiedIdeographsBCodePoints.first && unmatchedCodePoint <= cjkUnifiedIdeographsBCodePoints.second) ||
            (unmatchedCodePoint >= cjkUnifiedIdeographsCCodePoints.first && unmatchedCodePoint <= cjkUnifiedIdeographsCCodePoints.second) ||
            (unmatchedCodePoint >= cjkUnifiedIdeographsDCodePoints.first && unmatchedCodePoint <= cjkUnifiedIdeographsDCodePoints.second) ||
            (unmatchedCodePoint >= cjkUnifiedIdeographsECodePoints.first && unmatchedCodePoint <= cjkUnifiedIdeographsECodePoints.second) ||
            (unmatchedCodePoint >= cjkUnifiedIdeographsFCodePoints.first && unmatchedCodePoint <= cjkUnifiedIdeographsFCodePoints.second) ||
            (unmatchedCodePoint >= cjkUnifiedIdeographsGCodePoints.first && unmatchedCodePoint <= cjkUnifiedIdeographsGCodePoints.second))
        {
            // han (all other han unified ideographs)
            // these are ideographs that are in the following blocks:
            // 3400..4DBF; CJK Unified Ideographs Extension A
            // 20000..2A6DF; CJK Unified Ideographs Extension B
            // 2A700..2B73F; CJK Unified Ideographs Extension C
            // 2B740..2B81F; CJK Unified Ideographs Extension D
            // 2B820..2CEAF; CJK Unified Ideographs Extension E
            // 2CEB0..2EBEF; CJK Unified Ideographs Extension F
            // 30000..3134F; CJK Unified Ideographs Extension G
            aaaa = 0xfb80 + (unmatchedCodePoint >> 15);
            bbbb = (unmatchedCodePoint & 0x7fff) | 0x8000;
        }
        else
        {
            // unassigned
            aaaa = 0xfbc0 + (unmatchedCodePoint >> 15);
            bbbb = (unmatchedCodePoint & 0x7fff) | 0x8000;
        }

        uint64_t element0 = 0;
        element0 |= static_cast<uint64_t>(aaaa) << 24;
        element0 |= static_cast<uint64_t>(0x0020) << 8;
        element0 |= 0x02;
        uint64_t element1 = 0;
        element1 |= static_cast<uint64_t>(bbbb) << 24;

        std::vector<uint64_t> collationElements;
        collationElements.push_back(element0);
        collationElements.push_back(element1);

        return collationElements;
    }

    std::vector<uint64_t> FindCollationElement(const std::vector<uint32_t>& codePoints)
    {
        if (codePoints.size() > 1)
        {
            // this is a multi-code point match, so we need to hash it first
            uint32_t hashedSequence = ComputeHash(codePoints);
            uint16_t multiBlockIndex = hashedSequence / DUCET_MULTI_BLOCK_SIZE;
            if (unicodeDucetMultiMap.find(multiBlockIndex) != unicodeDucetMultiMap.end())
            {
                if (unicodeDucetMultiMap[multiBlockIndex]->find(hashedSequence) != unicodeDucetMultiMap[multiBlockIndex]->end())
                {
                    return unicodeDucetMultiMap[multiBlockIndex]->operator[](hashedSequence);
                }
            }
        }
        else
        {
            // single code point match
            uint16_t singleBlockIndex = codePoints[0] / DUCET_BLOCK_SIZE;
            if (unicodeDucetMap.find(singleBlockIndex) != unicodeDucetMap.end())
            {
                if (unicodeDucetMap[singleBlockIndex]->find(codePoints[0]) != unicodeDucetMap[singleBlockIndex]->end())
                {
                    std::vector<uint64_t> collationElement;
                    collationElement.push_back(unicodeDucetMap[singleBlockIndex]->operator[](codePoints[0]));

                    return collationElement;
                }
            }

            //  not in the single block table, could be in the multi block one though
            uint16_t multiBlockIndex = codePoints[0] / DUCET_MULTI_BLOCK_SIZE;
            if (unicodeDucetMultiMap.find(multiBlockIndex) != unicodeDucetMultiMap.end())
            {
                if (unicodeDucetMultiMap[multiBlockIndex]->find(codePoints[0]) != unicodeDucetMultiMap[multiBlockIndex]->end())
                {
                    return unicodeDucetMultiMap[multiBlockIndex]->operator[](codePoints[0]);
                }
            }

            // not in either table, have to derive an element
            return  DeriveCollationElement(codePoints[0]);
        }

        // no match, return an empty vector
        return std::vector<uint64_t>();
    }

    ///
    /// Retrieves the consecutive sequence of non-starter code points from \c normalizedSource starting at \c startingPosition. 
    /// 
    /// Returns a vector of code points representing all consecutive non-starters.
    /// 
    std::vector<uint32_t> GetNonStarters(const std::vector<uint32_t>& normalizedSource, const std::vector<uint32_t>::const_iterator& startingPosition)
    {
        std::vector<uint32_t> nonStarters;
        for (std::vector<uint32_t>::const_iterator it = startingPosition; it != normalizedSource.end(); it++)
        {
            if (unicodeCanonicalCombiningClass.find(*it) != unicodeCanonicalCombiningClass.end())
            {
                nonStarters.push_back(*it);
            }
            else
            {
                break;
            }
        }

        return nonStarters;
    }

    ///
    /// Determines if the sub-string represented by \c codePoints has a match
    /// in the collation element table. 
    /// 
    /// Returns true if there is a match, false otherwise.
    /// 
    inline bool HasCollationElementMatch(const std::vector<uint32_t>& codePoints)
    {
        if (codePoints.size() > 1)
        {
            // this is a multi-code point match, so we need to hash it first
            uint32_t hashedSequence = ComputeHash(codePoints);
            uint16_t multiBlockIndex = hashedSequence / DUCET_MULTI_BLOCK_SIZE;
            if (unicodeDucetMultiMap.find(multiBlockIndex) != unicodeDucetMultiMap.end())
            {
                return (unicodeDucetMultiMap[multiBlockIndex]->find(hashedSequence) != unicodeDucetMultiMap[multiBlockIndex]->end());
            }
        }
        else
        {
            // single code point match
            uint16_t singleBlockIndex = codePoints[0] / DUCET_BLOCK_SIZE;
            if (unicodeDucetMap.find(singleBlockIndex) != unicodeDucetMap.end())
            {
                if (unicodeDucetMap[singleBlockIndex]->find(codePoints[0]) != unicodeDucetMap[singleBlockIndex]->end())
                {
                    return true;
                }
            }

            //  not in the single block table, could be in the multi block one though
            uint16_t multiBlockIndex = codePoints[0] / DUCET_MULTI_BLOCK_SIZE;
            if (unicodeDucetMultiMap.find(multiBlockIndex) != unicodeDucetMultiMap.end())
            {
                return (unicodeDucetMultiMap[multiBlockIndex]->find(codePoints[0]) != unicodeDucetMultiMap[multiBlockIndex]->end());
            }
        }

        return false;
    }

    ///
    /// Determines if a codepoint is a non-starter.
    /// A non-starter is one that has a non-zero value for canonical combining class. 
    /// 
    /// Returns true if the codepoint is a non-starter, false otherwise.
    /// 
    inline bool IsNonStarter(uint32_t codePoint)
    {
        return (unicodeCanonicalCombiningClass.find(codePoint) != unicodeCanonicalCombiningClass.end());
    }

    ///
    /// Finds the longest codepoint substring that has a match in the collation element table
    /// while skipping all unblocked non-starters that don't contribute to the match. 
    ///
    /// A vector of codepoints representing the longest substring that has a match in the
    /// collation element table (including if the codepoint has a derived collation element).
    /// 
    std::vector<uint32_t> FindLongestSubstring(const std::vector<uint32_t>& normalizedSource, const std::vector<uint32_t>::const_iterator& startPosition, size_t& numCodePointsProcessed)
    {
        // the longest substring is defined as the longest sequence of:
        // 1. starters that have a match in the collation element table
        // 2. starters + non-starters (contiguous or discontiguous) 
        // 3. a sequence of non-starters with no starters in front
        std::vector<uint32_t> longestSubstring;
        numCodePointsProcessed = 0;

        // Case 1: Non-Starter Sequence
        if (IsNonStarter(*startPosition))
        {
            longestSubstring.push_back(*startPosition);
            numCodePointsProcessed++;
            for (std::vector<uint32_t>::const_iterator it = (startPosition + 1); it != normalizedSource.end(); it++)
            {
                if (IsNonStarter(*it))
                {
                    longestSubstring.push_back(*it);
                    if (HasCollationElementMatch(longestSubstring))
                    {
                        numCodePointsProcessed++;
                    }
                    else
                    {
                        longestSubstring.pop_back();
                        return longestSubstring;
                    }
                }
                else
                {
                    return longestSubstring;
                }
            }
        }

        // Case 2: Starter sequence
        for (std::vector<uint32_t>::const_iterator it = startPosition; it != normalizedSource.end(); it++)
        {
            if (!IsNonStarter(*it))
            {
                longestSubstring.push_back(*it);
                if (!HasCollationElementMatch(longestSubstring))
                {
                    // a special case is if the starter needs a derived collation element
                    // in this case it doesn't have a match, so nothing will be in the longest substring
                    // otherwise we've reached the end of our longest contiguous substring
                    if (longestSubstring.size() == 1)
                    {
                        numCodePointsProcessed++;
                    }
                    else
                    {
                        // no match, we've reached the end of our longest contiguous starter sequence
                        longestSubstring.pop_back();
                    }
                    
                    // we can return here because we've processed starters
                    // and the next one (the one we just checked) wasn't
                    // a non-starter
                    return longestSubstring;
                }

                numCodePointsProcessed++;
            }
            else
            {
                // it's a non-starter
                break;
            }
        }

        // Case 3: Starter sequence with matching unblocked non-starters
        // In this case we have to see if there is a discontiguous match up until we hit an end
        // if not, we have to roll back the non-starters that we've seen
        size_t numCodePointsToProcess = 0;
        uint16_t lastCombiningClass = 0;
        for (std::vector<uint32_t>::const_iterator it = (startPosition + numCodePointsProcessed); it != normalizedSource.end(); it++)
        {
            if (IsNonStarter(*it))
            {
                // is it blocking or non-blocking?
                if (lastCombiningClass == 0 || (unicodeCanonicalCombiningClass[*it] != lastCombiningClass))
                {
                    // non-blocking
                    lastCombiningClass = unicodeCanonicalCombiningClass[*it];
                    numCodePointsToProcess++;
                }
                else
                {
                    // blocking, we are done
                    break;
                }
            }
            else
            {
                // hit a starter, that's the end of the sequence
                break;
            }
        }

        // we need to process from (startPosition + numCodePointsProcessed) to (startPosition + numCodePointsProcessed + numCodePointsToProcess)
        std::vector<uint32_t>::const_iterator lastMatchPosition = normalizedSource.end();
        for (std::vector<uint32_t>::const_iterator it = (startPosition + numCodePointsProcessed); (it != normalizedSource.end()) && (numCodePointsToProcess > 0); it++)
        {
            longestSubstring.push_back(*it);
            if (HasCollationElementMatch(longestSubstring))
            {
                // S = S + C - leave what's in longestSubstring
                lastMatchPosition = it;
            }
            else
            {
                longestSubstring.pop_back();
            }

            numCodePointsToProcess--;
        }

        // the number of characters we processed in total is equal to the difference between the last match position and the start
        if (lastMatchPosition != normalizedSource.end())
        {
            numCodePointsProcessed += static_cast<size_t>(lastMatchPosition - (startPosition + numCodePointsProcessed) + 1);
        }

        return longestSubstring;
    }

    ///
    /// Produces a collation element array for a normalized vector of code points \c normalizedSource.
    /// Returns a vector where each sub-element represents the collation element for an individual 
    /// character in the normalized source (consolidated primary, secondary, and tertiary weights).
    /// 
    /// Note that this implementation ignores Variable Weighting
    ///
    std::vector<uint64_t> Collate(const std::vector<uint32_t>& normalizedSource)
    {
        size_t numProcessed = 0;
        std::vector<uint64_t> result;
        std::vector<uint32_t>::const_iterator it = normalizedSource.begin();
        while (it < normalizedSource.end())
        {
            std::vector<uint32_t> longestSubstring = FindLongestSubstring(normalizedSource, it, numProcessed);
            std::vector<uint64_t> collationElements = FindCollationElement(longestSubstring);
            result.insert(result.end(), collationElements.begin(), collationElements.end());

            // advance the iterator
            it = (it + numProcessed);
        }

        return result;
    }

    ///
    /// Constructs a sort key from a collation element array formed from a UTF-8 encoded Unicode string.
    ///
    std::vector<uint16_t> ConstructSortKey(const std::vector<uint64_t>& collationElementArray)
    {
        // we construct a sort key by successively appending all non-zero weights from the collation element array
        std::vector<uint16_t> result;

        // first append all primary weights
        for (size_t i = 0; i < collationElementArray.size(); i++)
        {
            //uint16_t primaryWeight = static_cast<uint16_t>((collationElementArray[i] & 0xFFFF0000) >> 16);
            uint16_t primaryWeight = static_cast<uint16_t>((collationElementArray[i] >> 24) & 0xFFFF);
            if (primaryWeight > 0)
            {
                result.push_back(primaryWeight);
            }
        }

        // at the end of each level, append a level separator
        result.push_back(0);

        // then all secondary weights
        for (size_t i = 0; i < collationElementArray.size(); i++)
        {
            //uint16_t secondaryWeight = static_cast<uint16_t>((collationElementArray[i] & 0xFFFFFFFF00000000) >> 8);
            uint16_t secondaryWeight = static_cast<uint16_t>((collationElementArray[i] >> 8) & 0xFFFF);
            if (secondaryWeight > 0)
            {
                result.push_back(secondaryWeight);
            }
        }

        // at the end of each level, append a level separator
        result.push_back(0);

        // then all tertiary weights
        for (size_t i = 0; i < collationElementArray.size(); i++)
        {
            uint16_t tertiaryWeight = static_cast<uint16_t>(collationElementArray[i] & 0xFF);
            if (tertiaryWeight > 0)
            {
                result.push_back(tertiaryWeight);
            }
        }

        return result;
    }

    bool TfUTF8UCALessThan::operator()(const std::string& lhs, const std::string& rhs) const
    {
        // NOTE: this is a naive implementation of UCA, there are many ways
        // to optimize this (many given in Section 9 of Unicode TR10)
        
        // Step 1: Normalize both strings to normalization form D (NFD)
        // this consists of going through each code point and determining
        // whether that code point decomposes into a set of additional code points
        // (each of much be evaluated recursively) - super expensive
        std::vector<uint32_t> normalizedLhs = Normalize(lhs);
        std::vector<uint32_t> normalizedRhs = Normalize(rhs);

        // Step 2: Produce Collation Element Arrays
        std::vector<uint64_t> collationElementArrayLhs = Collate(normalizedLhs);
        std::vector<uint64_t> collationElementArrayRhs = Collate(normalizedRhs);

        // Step 3: Construct a sort key for each collation element
        std::vector<uint16_t> sortKeyLhs = ConstructSortKey(collationElementArrayLhs);
        std::vector<uint16_t> sortKeyRhs = ConstructSortKey(collationElementArrayRhs);
        
        // Step 4: Compare sort keys
        std::size_t compareLength = std::min(sortKeyLhs.size(), sortKeyRhs.size());
        for (size_t i = 0; i < compareLength; i++)
        {
            if (sortKeyLhs[i] < sortKeyRhs[i])
            {
                // left hand string is UCA less than right hand string
                return true;
            }
            else if (sortKeyRhs[i] < sortKeyLhs[i])
            {
                // right hand string is UCA less than left hand string
                return false;
            }
        }

        // if we get to the end of the loop, we've exhausted the shorter one
        // which means that one is the one that is "less" (note that if 
        // the sizes were the same and the keys were the same, they are the same
        // and the order returned here doesn't matter)
        return (sortKeyLhs.size() < sortKeyRhs.size()) ? true : false;
    }

} // end namepsace TfUnicodeUtils

PXR_NAMESPACE_CLOSE_SCOPE