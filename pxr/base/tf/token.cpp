//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/token.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/hashset.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

#include "pxr/base/arch/align.h"

#include <tbb/spin_mutex.h>

#include <atomic>
#include <algorithm>
#include <cstring>
#include <new>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

using std::vector;
using std::string;
using std::pair;

PXR_NAMESPACE_OPEN_SCOPE

struct Tf_TokenRegistry
{
    typedef TfToken::_Rep const *_RepPtr;
    typedef TfPointerAndBits<const TfToken::_Rep> _RepPtrAndBits;

    struct _Eq { 
        inline size_t operator()(TfToken::_Rep const &rep1,
                                 TfToken::_Rep const &rep2) const {
            return (*this)(rep1._cstr, rep2._cstr);
        }
        inline bool operator()(const char* s1, const char* s2) const {
            return !strcmp(s1, s2);
        }
    };

    template <int Mul>
    struct _Hash {
        inline size_t operator()(TfToken::_Rep const &rep) const {
            return (*this)(rep._cstr);
        }
        inline size_t operator()(char const *s) const {
            // Matches STL original implementation for now.  Switch to something
            // better?
            unsigned int h = 0;
            for (; *s; ++s)
                h = Mul * h + *s;
            return h;
        }
    };

    typedef _Hash<7> _OuterHash;
    typedef _Hash<5> _InnerHash;

    static constexpr unsigned _NumSets = 128;
    static constexpr unsigned _SetMask = _NumSets-1;
    static constexpr size_t _MinInsertsUntilSweepCheck = 32;

    // TODO: Consider a better container than TfHashSet here.
    using _RepSet = TfHashSet<TfToken::_Rep, _InnerHash, _Eq>;

    // Align this structure to ensure no false sharing of 'mutex'.
    struct alignas(ARCH_CACHE_LINE_SIZE)
    _Set {
        _RepSet reps;
        unsigned insertsUntilSweepCheck = _MinInsertsUntilSweepCheck;
        mutable tbb::spin_mutex mutex;
    };

    // Data members.
    _Set _sets[_NumSets];

    static Tf_TokenRegistry& _GetInstance() {
        return TfSingleton<Tf_TokenRegistry>::GetInstance();
    }
 
    inline size_t _GetSetNum(char const *s) const {
        _OuterHash h;
        return h(s) & _SetMask;
    }

    inline _RepPtrAndBits _GetPtrChar(char const *s, bool makeImmortal) {
        return _GetPtrImpl(s, makeImmortal);
    }
    inline _RepPtrAndBits _GetPtrStr(string const &s, bool makeImmortal) {
        // Explicitly specify template arg -- if unspecified, it will deduce
        // string by-value, which we don't want.
        return _GetPtrImpl<string const &>(s, makeImmortal);
    }

    inline _RepPtrAndBits _FindPtrChar(char const *s) const {
        return _FindPtrImpl(s);
    }
    inline _RepPtrAndBits _FindPtrStr(string const &s) const {
        // Explicitly specify template arg -- if unspecified, it will deduce
        // string by-value, which we don't want.
        return _FindPtrImpl<string const &>(s);
    }

    void _DumpStats() const {
        std::vector<std::pair<size_t, size_t> > sizesWithSet;
        for (size_t i = 0; i != _NumSets; ++i) {
            tbb::spin_mutex::scoped_lock lock(_sets[i].mutex);
            sizesWithSet.push_back(std::make_pair(_sets[i].reps.size(), i));
        }
        std::sort(sizesWithSet.begin(), sizesWithSet.end());
        printf("Set # -- Size\n");
        TF_FOR_ALL(i, sizesWithSet) {
            printf("%zu -- %zu\n", i->second, i->first);
        }

        // Uncomment to dump every token & refcount.
        // for (size_t i = 0; i != _NumSets; ++i) {
        //     tbb::spin_mutex::scoped_lock lock(_sets[i].mutex);
        //     for (auto const &rep: _sets[i].reps) {
        //         printf("%d '%s'\n", rep._refCount.load(), rep._str.c_str());
        //     }
        // }
        
    }

private:

    inline bool _IsEmpty(char const *s) const { return !s || !s[0]; }
    inline bool _IsEmpty(string const &s) const { return s.empty(); }

    inline char const *_CStr(char const *s) const { return s; }
    inline char const *_CStr(string const &s) const { return s.c_str(); }

    static TfToken::_Rep _LookupRep(char const *cstr) {
        TfToken::_Rep ret;
        ret._cstr = cstr;
        return ret;
    }

    static inline uint64_t _ComputeCompareCode(char const *p) {
        uint64_t compCode = 0;
        int nchars = sizeof(compCode);
        while (nchars--) {
            compCode |= static_cast<uint64_t>(*p) << (8*nchars);
            if (*p) {
                ++p;
            }
        }
        return compCode;
    }

    /*
     * Either finds a key that is stringwise-equal to s,
     * or puts a new _Rep into the map for s.
     */
    template <class Str>
    inline _RepPtrAndBits _GetPtrImpl(Str s, bool makeImmortal) {
        if (_IsEmpty(s)) {
            return _RepPtrAndBits();
        }

        unsigned setNum = _GetSetNum(_CStr(s));
        _Set &set = _sets[setNum];

        tbb::spin_mutex::scoped_lock lock(set.mutex);

        // Insert or lookup an existing.
        _RepSet::iterator iter = set.reps.find(_LookupRep(_CStr(s)));
        if (iter != set.reps.end()) {
            _RepPtr rep = &(*iter);
            bool isCounted = rep->_refCount.load(std::memory_order_relaxed) & 1;
            if (isCounted) {
                if (makeImmortal) {
                    // Clear counted flag.
                    rep->_refCount.fetch_and(~1u, std::memory_order_relaxed);
                    isCounted = false;
                }
                else {
                    // Add one reference (we count by 2's).
                    rep->_refCount.fetch_add(2, std::memory_order_relaxed);
                }
            }
            return _RepPtrAndBits(rep, isCounted);
        } else {
            // No entry present, add a new entry.  First check if we should
            // clean house.
            if (set.insertsUntilSweepCheck == 0) {
                // If this insert would cause a rehash, clean house first.
                // Remove any reps that have refcount zero.  This is safe to do
                // since we hold a lock on the set's mutex: no other thread can
                // observe or manipulate this set concurrently.
                if ((float(set.reps.size() + 1) /
                     float(set.reps.bucket_count()) >
                     set.reps.max_load_factor())) {
                    // Walk the set and erase any non-immortal reps with no
                    // outstanding refcounts (_refCount == 1).  Recall that the
                    // low-order bit indicates counted (non-immortal) reps, we
                    // count references by 2.
                    for (auto iter = set.reps.begin(),
                             end = set.reps.end(); iter != end; ) {
                        if (iter->_refCount == 1) {
                            set.reps.erase(iter++);
                        }
                        else {
                            ++iter;
                        }
                    }
                    // Reset insertsUntilSweepCheck.  We want to wait at least a
                    // bit to avoid bad patterns.  For example if we sweep the
                    // table and find one garbage token, we will remove it and
                    // then insert the new token.  On the next insert we'll
                    // sweep for garbage again.  If the calling code is
                    // consuming a list of tokens, discarding one and creating a
                    // new one on each iteration, we could end up sweeping the
                    // whole table like this on every insertion.  By waiting a
                    // bit we avoid these situations at the cost of the
                    // occasional "unnecessary" rehash.
                    set.insertsUntilSweepCheck = std::max(
                        _MinInsertsUntilSweepCheck,
                        static_cast<size_t>(
                            set.reps.bucket_count() *
                            float(set.reps.max_load_factor() -
                                  set.reps.load_factor())));
                }
            }
            else {
                --set.insertsUntilSweepCheck;
            }
                
            // Create the new entry.
            TfAutoMallocTag noname("TfToken");
            uint64_t compareCode = _ComputeCompareCode(_CStr(s));
            _RepPtr rep = &(*set.reps.insert(
                                TfToken::_Rep(s, setNum, compareCode)).first);
            // 3, because 1 for counted bit plus 2 for one refcount.  We can
            // store relaxed here since our lock on the set's mutex provides
            // synchronization.
            rep->_refCount.store(makeImmortal ? 0 : 3,
                                 std::memory_order_relaxed);
            return _RepPtrAndBits(rep, !makeImmortal);
        }
    }

    template <class Str>
    _RepPtrAndBits _FindPtrImpl(Str s) const {
        if (_IsEmpty(s)) {
            return _RepPtrAndBits();
        }
        
        size_t setNum = _GetSetNum(_CStr(s));
        _Set const &set = _sets[setNum];

        tbb::spin_mutex::scoped_lock lock(set.mutex);

        _RepSet::const_iterator iter = set.reps.find(_LookupRep(_CStr(s)));
        if (iter == set.reps.end()) {
            return _RepPtrAndBits();
        }

        TfToken::_Rep const *rep = &(*iter);
        
        unsigned oldVal =
            rep->_refCount.fetch_add(2, std::memory_order_relaxed);

        return _RepPtrAndBits(rep, oldVal & 1);
    }

};

TF_INSTANTIATE_SINGLETON(Tf_TokenRegistry);

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<TfToken>();
    TfType::Define< vector<TfToken> >()
        .Alias( TfType::GetRoot(), "vector<TfToken>" );
}

string const&
TfToken::_GetEmptyString()
{
    static std::string empty;
    return empty;
}

TfToken::TfToken(const string &s)
    : _rep(Tf_TokenRegistry::_GetInstance().
           _GetPtrStr(s, /*makeImmortal*/false))
{
}

TfToken::TfToken(const char *s)
    : _rep(Tf_TokenRegistry::_GetInstance().
           _GetPtrChar(s, /*makeImmortal*/false))
{
}

TfToken::TfToken(const string &s, _ImmortalTag)
    : _rep(Tf_TokenRegistry::_GetInstance().
           _GetPtrStr(s, /*makeImmortal*/true))
{
}

TfToken::TfToken(const char *s, _ImmortalTag)
    : _rep(Tf_TokenRegistry::_GetInstance().
           _GetPtrChar(s, /*makeImmortal*/true))
{
}

TfToken
TfToken::Find(const string& s)
{
    TfToken t;
    t._rep = Tf_TokenRegistry::_GetInstance()._FindPtrStr(s);
    return t;
}

bool 
TfToken::operator==(const string &o) const
{
    return GetString() == o;
}

bool
TfToken::operator==(const char *o) const
{
    return GetString() == o;
}

std::vector<TfToken>
TfToTokenVector(const std::vector<string> &sv)
{
    return vector<TfToken>(sv.begin(), sv.end());
}

std::vector<string>
TfToStringVector(const std::vector<TfToken> &tv)
{
    std::vector<string> sv(tv.size());

    for (size_t i = 0; i != tv.size(); i++)
        sv[i] = tv[i].GetString();

    return sv;
}

std::ostream &
operator <<(std::ostream &stream, const TfToken& token)
{
    return stream << token.GetText();
}

TF_API void TfDumpTokenStats()
{
    Tf_TokenRegistry::_GetInstance()._DumpStats();
}

PXR_NAMESPACE_CLOSE_SCOPE
