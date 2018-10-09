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

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/token.h"

#include "pxr/base/tf/hashset.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"

#include "pxr/base/arch/align.h"

#include <tbb/spin_mutex.h>

#include <string>
#include <ostream>
#include <vector>
#include <cstring>
#include <utility>

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

    static const size_t _NumSets = 128;
    static const size_t _SetMask = _NumSets-1;
    
    // Utility to pad an instance to take up a cache line to avoid false
    // sharing.
    template <class T>
    struct alignas(ARCH_CACHE_LINE_SIZE) _CacheLinePadded {
        T val;
    };

    // It's not ideal to use TfHashSet here.  It would be better to use
    // boost::multi_index from boost 1.56 or newer: it lets us lookup an element
    // by a key more easily.  It also has better performance.  Earlier versions
    // suffer from the issue where erase(iterator) has to return an iterator to
    // the next element, which has to scan forward to find the next, so we use
    // TfHashSet for now.
    typedef TfHashSet<TfToken::_Rep, _InnerHash, _Eq> _RepSet;

    // Data members.
    _RepSet _sets[_NumSets];
    mutable _CacheLinePadded<tbb::spin_mutex> _locks[_NumSets];

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

    /*
     * rep may be dying.  remove its key and raw pointer reference
     * from the table if it truly is.
     */
    void _PossiblyDestroyRep(_RepPtr rep) {
        bool repFoundInSet = true;
        string repString;
        {
            unsigned int setNum = rep->_setNum;

            tbb::spin_mutex::scoped_lock lock(_locks[setNum].val);

            if (!rep->_isCounted)
                return; 
            
            /*
             * We hold the lock, but there could be others outside the lock
             * modifying this same counter.  Be safe: be atomic.
             */
            if (--rep->_refCount != 0)
                return;
            
            if (!_sets[setNum].erase(*rep)) {
                repFoundInSet = false;
                repString = rep->_str;
            }            
        }
        TF_VERIFY(repFoundInSet,
                  "failed to find token '%s' in table for destruction",
                  repString.c_str());
    }

    void _DumpStats() const {
        std::vector<std::pair<size_t, size_t> > sizesWithSet;
        for (size_t i = 0; i != _NumSets; ++i) {
            sizesWithSet.push_back(std::make_pair(_sets[i].size(), i));
        }
        std::sort(sizesWithSet.begin(), sizesWithSet.end());
        printf("Set # -- Size\n");
        TF_FOR_ALL(i, sizesWithSet) {
            printf("%zu -- %zu\n", i->second, i->first);
        }
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

    /*
     * Either finds a key that is stringwise-equal to s,
     * or puts a new _Rep into the map for s.
     */
    template <class Str>
    inline _RepPtrAndBits _GetPtrImpl(Str s, bool makeImmortal) {
        if (_IsEmpty(s))
            return _RepPtrAndBits();

        size_t setNum = _GetSetNum(_CStr(s));

        tbb::spin_mutex::scoped_lock lock(_locks[setNum].val);

        // Insert or lookup an existing.
        _RepSet::iterator iter = _sets[setNum].find(_LookupRep(_CStr(s)));
        if (iter != _sets[setNum].end()) {
            _RepPtr rep = &(*iter);
            bool isCounted = rep->_isCounted;
            if (isCounted) {
                if (makeImmortal)
                    isCounted = rep->_isCounted = false;
                else
                    ++rep->_refCount;
            }
            return _RepPtrAndBits(rep, isCounted);
        } else {
            // No entry present, add a new entry.
            TfAutoMallocTag noname("TfToken");
            _RepPtr rep = &(*_sets[setNum].insert(TfToken::_Rep(s)).first);
            rep->_isCounted = !makeImmortal;
            rep->_setNum = setNum;
            if (!makeImmortal)
                rep->_refCount = 1;
            return _RepPtrAndBits(rep, !makeImmortal);
        }
    }

    template <class Str>
    _RepPtrAndBits _FindPtrImpl(Str s) const {
        if (_IsEmpty(s))
            return _RepPtrAndBits();
        
        size_t setNum = _GetSetNum(_CStr(s));

        tbb::spin_mutex::scoped_lock lock(_locks[setNum].val);

        _RepSet::const_iterator iter =
            _sets[setNum].find(_LookupRep(_CStr(s)));
        if (iter == _sets[setNum].end())
            return _RepPtrAndBits();

        return _RepPtrAndBits(&(*iter), iter->IncrementIfCounted());
    }

};

TF_INSTANTIATE_SINGLETON(Tf_TokenRegistry);

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<TfToken>();
    TfType::Define< vector<TfToken> >()
        .Alias( TfType::GetRoot(), "vector<TfToken>" );
}

void
TfToken::_PossiblyDestroyRep() const
{
    Tf_TokenRegistry::_GetInstance()._PossiblyDestroyRep(_rep.Get());
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
