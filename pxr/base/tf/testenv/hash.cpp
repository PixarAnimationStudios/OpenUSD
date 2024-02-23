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
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/weakPtr.h"

#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

struct Two
{
    uint32_t x, y;
};

template <class HashState>
void TfHashAppend(HashState &h, Two two)
{
    h.Append(two.x,
             two.y);
}

template <class Hasher>
static inline void
TestTwo(Hasher const &h, Two t, unsigned *counts)
{
    // Hash x, then flip each bit in x and hash again.
    // For each flipped bit in the resulting hash, increment a counter.
    uint64_t tHash = h(t);
    for (int i = 0; i != 32; ++i) {
        Two tPrime = t;
        tPrime.x ^= (1 << i);
        uint64_t tPrimeHash = h(tPrime);
        uint64_t flips = tHash ^ tPrimeHash;
        for (int index = 0; flips; ++index, flips >>= 1) {
            if (flips & 1) {
                ++counts[index];
            }
        }
    }
    for (int i = 0; i != 32; ++i) {
        Two tPrime = t;
        tPrime.y ^= (1 << i);
        uint64_t tPrimeHash = h(tPrime);
        uint64_t flips = tHash ^ tPrimeHash;
        for (int index = 0; flips; ++index, flips >>= 1) {
            if (flips & 1) {
                ++counts[index];
            }
        }
    }
}

template <class Hasher>
static inline void
TestOne(Hasher const &h, uint64_t x, unsigned *counts)
{
    // Hash x, then flip each bit in x and hash again.
    // For each flipped bit in the resulting hash, increment a counter.

    uint64_t xHash = h(x);
    for (int i = 0; i != 64; ++i) {
        uint64_t xPrime = x ^ (1 << i);
        uint64_t xPrimeHash = h(xPrime);
        uint64_t flips = xHash ^ xPrimeHash;
        for (int index = 0; flips; ++index, flips >>= 1) {
            if (flips & 1) {
                ++counts[index];
            }
        }
    }
}

template <class Hasher>
static void
_TestStatsOne(Hasher const &h, char const *label)
{
    TfStopwatch sw;
    sw.Start();

    constexpr uint64_t NTESTS = 100000;
    uint64_t numTests = NTESTS;

    unsigned counts[64] {0};

    while (numTests--) {
        uint64_t num = numTests << 5; //((uint64_t)rand() << 32) + rand();
        TestOne(h, num, counts);
    }

    printf("%s One: %zu tests.\n", label, NTESTS * 64);
    for (int i = 0; i != 64; ++i) {
        printf("bit %d flipped %d times (%.2f%%)\n", i, counts[i],
               100.0 * double(counts[i]) / (double(NTESTS) * 64.0));
    }
    
    sw.Stop();
    printf("took %f seconds\n", sw.GetSeconds());
}

template <class Hasher>
static void
_TestStatsTwo(Hasher const &h, char const *label)
{
    TfStopwatch sw;
    sw.Start();

    constexpr uint64_t NTESTS = 100000;
    uint64_t numTests = NTESTS;

    unsigned counts[64] {0};

    while (numTests--) {
        Two t { static_cast<uint32_t>(numTests << 5),
                static_cast<uint32_t>(numTests >> 5) };
        TestTwo(h, t, counts);
    }

    printf("%s Two: %zu tests.\n", label, NTESTS * 64);
    for (int i = 0; i != 64; ++i) {
        printf("bit %d flipped %d times (%.2f%%)\n", i, counts[i],
               100.0 * double(counts[i]) / (double(NTESTS) * 64.0));
    }
    sw.Stop();
    printf("took %f seconds\n", sw.GetSeconds());
}

struct TfHasher
{
    size_t operator()(uint64_t x) const {
        return TfHash()(x);
    }
    
    size_t operator()(Two t) const {
        return TfHash()(t);
    }
};

class Dolly : public TfRefBase, public TfWeakBase {
public:
    typedef TfRefPtr<Dolly> DollyRefPtr;
    typedef TfWeakPtr<Dolly> DollyPtr;
    static DollyRefPtr New() {
       // warning: return new Dolly directly will leak memory!
       return TfCreateRefPtr(new Dolly);
    }

    ~Dolly() {};

 private:
    Dolly() {};
    Dolly(bool) {};
};


// Ensure that types that implicitly convert to bool/int will not hash with
// TfHash.
struct _NoHashButConvertsToBool
{
    operator bool() { return true; }
};

struct _NoHashButConvertsToInt
{
    operator int() { return 123; }
};

template <class T, class = decltype(TfHash()(std::declval<T>()))>
constexpr bool _IsHashable(int) { return true; }
template <class T>
constexpr bool _IsHashable(...) { return false; }

static_assert(!_IsHashable<_NoHashButConvertsToBool>(0), "");
static_assert(!_IsHashable<_NoHashButConvertsToInt>(0), "");
static_assert(_IsHashable<bool>(0), "");
static_assert(_IsHashable<int>(0), "");

struct MultipleThings
{
    int ival = 123;
    float fval = 1.23f;
    std::string sval = "123";
    std::vector<int> vints = { 1, 2, 3 };
    std::set<float> sfloats = { 1.2f, 2.3f, 3.4f };
};

template <class HashState>
void
TfHashAppend(HashState &h, MultipleThings const &mt)
{
    h.Append(mt.ival,
             mt.fval,
             mt.sval,
             mt.vints);
    h.AppendRange(mt.sfloats.begin(), mt.sfloats.end());
}

static bool
Test_TfHash()
{
    Dolly::DollyRefPtr ref = Dolly::New();

    TfHash h;

    printf("hash(TfRefPtr): %zu\n", h(ref));

    Dolly::DollyPtr weak(ref);
    printf("hash(TfWeakPtr): %zu\n", h(weak));


    TfToken tok("hello world");
    printf("hash(TfToken): %zu\n", h(tok));

    std::string str("hello world");
    printf("hash(std::string): %zu\n", h(str));

    printf("hash(float zero): %zu\n", h(-0.0f));
    printf("hash(float neg zero): %zu\n", h(0.0f));
    printf("hash(double zero): %zu\n", h(-0.0));
    printf("hash(double neg zero): %zu\n", h(0.0));

    enum {
        FooA, FooB, FooC
    } fooEnum;

    static_assert(_IsHashable<decltype(fooEnum)>(0), "");
    printf("hash(FooEnum): %zu\n", h(FooA));
    printf("hash(FooEnum): %zu\n", h(FooB));
    printf("hash(FooEnum): %zu\n", h(FooC));

    enum : char {
        BarA, BarB, BarC
    } barEnum;

    static_assert(_IsHashable<decltype(barEnum)>(0), "");
    printf("hash(BarEnum): %zu\n", h(BarA));
    printf("hash(BarEnum): %zu\n", h(BarB));
    printf("hash(BarEnum): %zu\n", h(BarC));

    for (int order = 10; order != 1000000; order *= 10) {
        for (int i = 0; i != order; i += order / 10) {
            printf("hash %d: %zu\n", i, h(i));
        }
    }

    std::vector<int> vint = {1, 2, 3, 4, 5};
    printf("hash(vector<int>): %zu\n", h(vint));

    std::vector<bool> vbool = {true, false, true};
    printf("hash(vector<bool>): %zu\n", h(vbool));

    std::set<int> sint = {1, 2, 3, 4, 5};
    printf("hash(set<int>): %zu\n", h(sint));

    std::map<int, uint32_t> mint = {{-1, 1}, {2, 3}, {-4, 5}};
    printf("hash(map<int, uint32_t>): %zu\n", h(mint));

    std::pair<int, float> intfloat = {1, 2.34};
    printf("hash(pair<int, float>): %zu\n", h(intfloat));

    std::vector<std::pair<int, float>> vp { intfloat, intfloat, intfloat };
    printf("hash(vector<pair<int, float>>): %zu\n", h(vp));

    MultipleThings mt;
    printf("hash(MultipleThings): %zu\n", h(mt));

    printf("combine hash of the 3: %zu\n",
           TfHash::Combine(vint, intfloat, vp));

    // Validate support for std::type_index
    printf("hash(type_index): %zu\n", h(std::type_index(typeid(int))));

    // Validate support for std::shared_ptr
    printf("hash(shared_ptr): %zu\n", h(std::make_shared<int>(5)));

    // Validate support for std::unique_ptr
    printf("hash(unique_ptr): %zu\n", h(std::make_unique<int>(7)));

    // Validate support for std::optional
    printf("hash(optional): %zu\n", h(std::make_optional<std::string>("xyz")));
    TF_AXIOM(h(std::optional<std::string>("xyz")) ==
             h(std::optional<std::string>("xyz")));

    // Validate support for std::variant
    printf("hash(variant): %zu\n",
           h(std::variant<std::string, int, double>("abc")));
    TF_AXIOM(h(std::variant<std::string, int, double>("abc")) ==
             h(std::variant<std::string, int, double>("abc")));

    TfHasher tfh;

    _TestStatsOne(tfh, "TfHash");
    _TestStatsTwo(tfh, "TfHash");

    bool status = true;
    return status;
}

TF_ADD_REGTEST(TfHash);

