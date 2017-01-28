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
#include "pxr/usd/usdAbc/alembicTest.h"
#include "pxr/usd/usdAbc/alembicData.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/base/tf/ostreamMethods.h"
#include <algorithm>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


template <class T>
static bool _Truncate(VtValue& v, int max = 5)
{
    if (v.IsHolding<VtArray<T> >()) {
        const VtArray<T> array = v.UncheckedGet<VtArray<T> >();
        if (array.size() > max) {
            VtArray<T> newArray(max);
            std::copy(array.begin(), array.begin() + max, newArray.begin());
            v = newArray;
            return true;
        }
    }
    return false;
}

// Wraps another visitor, feeding it specs in lexicographic order.
class UsdAbc_SortedDataSpecVisitor : public SdfAbstractDataSpecVisitor {
public:
    explicit UsdAbc_SortedDataSpecVisitor(SdfAbstractDataSpecVisitor* wrapped)
        : _visitor(wrapped) {}
    virtual ~UsdAbc_SortedDataSpecVisitor() {}

    // SdfAbstractDataSpecVisitor overrides
    virtual bool VisitSpec(const SdfAbstractData& data,
                           const SdfAbstractDataSpecId& id) {
        if (_visitor)
            _ids.push_back(_SpecId(id));
        return true;
    }

    virtual void Done(const SdfAbstractData& data) {
        if (_visitor) {
            // Sort ids.
            std::sort(_ids.begin(), _ids.end());
            
            // Pass ids to the wrapped visitor.
            for (const auto& id : _ids) {
                if (_Pass(data, id)) {
                    if (!_visitor->VisitSpec(data, id)) {
                        break;
                    }
                }
            }
            
            // Finish up.
            _visitor->Done(data);
            _ids.clear();
        }
    }

protected:
    /// Iff this returns \c true, \p id is passed to the wrapped visitor.
    /// The default returns \c true.
    virtual bool _Pass(const SdfAbstractData& data,
                       const SdfAbstractDataSpecId& id) {
        return true;
    }        

private:
    struct _SpecId {
        SdfPath propertyOwningSpecPath;
        TfToken propertyName;

        _SpecId(const SdfAbstractDataSpecId& id) :
            propertyOwningSpecPath(id.GetPropertyOwningSpecPath()),
            propertyName(id.GetPropertyName())
        {
            // Do nothing
        }

        operator SdfAbstractDataSpecId() const
        {
            return SdfAbstractDataSpecId(&propertyOwningSpecPath,&propertyName);
        }

        bool operator<(const _SpecId& rhs) const
        {
            // Sort by propertyOwningSpecPath then propertyName.  Note
            // that this is different than sorting on the full path:
            // that may sort a spec's properties after the spec's
            // namespace descendants.  This will sort properties before
            // namespace descendants.
            if (propertyOwningSpecPath < rhs.propertyOwningSpecPath) {
                return true;
            }
            if (rhs.propertyOwningSpecPath < propertyOwningSpecPath) {
                return false;
            }
            // NOTE: We could sort arbitrarily here if property order
            //       is unimportant.
            return propertyName < rhs.propertyName;
        }
    };

private:
    SdfAbstractDataSpecVisitor* _visitor;

    typedef std::vector<_SpecId> _SpecIds;
    _SpecIds _ids;
};



// Note that this works because the Alembic data visits in hierarchy order.
struct UsdAbc_AlembicWriteVisitor : public SdfAbstractDataSpecVisitor {
    virtual bool VisitSpec(const SdfAbstractData& data, 
                           const SdfAbstractDataSpecId& id)
    {
        const SdfPath& path = id.GetFullSpecPath();
        if (path == SdfPath::AbsoluteRootPath()) {
            // Ignore.
        }
        else {
            fprintf(stdout, "%*s", 2*int(path.GetPathElementCount()-1), "");
            if (id.IsProperty()) {
                VtValue custom = data.Get(id, SdfFieldKeys->Custom);
                if (custom.IsHolding<bool>()) {
                    fprintf(stdout, "%s",
                            custom.UncheckedGet<bool>() ? "custom " : "");
                }
                else if (!custom.IsEmpty()) {
                    fprintf(stdout, "!BAD_CUSTOM ");
                }

                VtValue typeName = data.Get(id, SdfFieldKeys->TypeName);
                if (typeName.IsHolding<TfToken>()) {
                    fprintf(stdout, "%s ", TfStringify(typeName).c_str());
                }
                else if (!typeName.IsEmpty()) {
                    fprintf(stdout, "!BAD_TYPE ");
                }

                fprintf(stdout, "%s", path.GetName().c_str());

                VtValue value = data.Get(id, SdfFieldKeys->Default);
                if (!value.IsEmpty()) {
                    // Truncate shaped types to not dump too much data.
                    const char* trailing = NULL;
                    if (value.IsArrayValued()) {
                        if (_Truncate<bool>(value) ||
                            _Truncate<double>(value) ||
                            _Truncate<float>(value) ||
                            _Truncate<GfMatrix2d>(value) ||
                            _Truncate<GfMatrix3d>(value) ||
                            _Truncate<GfMatrix4d>(value) ||
                            _Truncate<GfVec2d>(value) ||
                            _Truncate<GfVec2f>(value) ||
                            _Truncate<GfVec2i>(value) ||
                            _Truncate<GfVec3d>(value) ||
                            _Truncate<GfVec3f>(value) ||
                            _Truncate<GfVec3i>(value) ||
                            _Truncate<GfVec4d>(value) ||
                            _Truncate<GfVec4f>(value) ||
                            _Truncate<GfVec4i>(value) ||
                            _Truncate<int>(value) ||
                            _Truncate<SdfAssetPath>(value) ||
                            _Truncate<std::string>(value) ||
                            _Truncate<TfToken>(value)) {
                            trailing = "...";
                        }
                    }
                    std::string s = TfStringify(value);
                    if (trailing) {
                        s.insert(s.size() - 1, trailing);
                    }
                    if (value.IsHolding<std::string>()) {
                        s = '"' + s + '"';
                    }
                    fprintf(stdout, " = %s\n", s.c_str());
                }

                VtValue samples = data.Get(id, SdfFieldKeys->TimeSamples);
                std::set<double> times = data.ListTimeSamplesForPath(id);
                if (samples.IsEmpty()) {
                    if (times.size() <= 1) {
                        // Expected.
                    }
                    else {
                        fprintf(stdout, "%*s",
                                2*int(path.GetPathElementCount()-1), "");
                        fprintf(stdout, "!NO_SAMPLES, want %zd\n",
                                times.size());
                    }
                }
                else if (samples.IsHolding<SdfTimeSampleMap>()) {
                    const SdfTimeSampleMap& samplesMap =
                        samples.UncheckedGet<SdfTimeSampleMap>();
                    if (times.size() != samplesMap.size()) {
                        fprintf(stdout, "%*s",
                                2*int(path.GetPathElementCount()-1), "");
                        fprintf(stdout, "!SAMPLES_MISMATCH, "
                                "have %zd vs want %zd\n",
                                samplesMap.size(), times.size());
                    }
                    else {
                        // XXX: Should compare times in samplesMap and
                        //      times.
                        fprintf(stdout, "%*s",
                                2*int(path.GetPathElementCount()-1), "");
                        fprintf(stdout, "samples_at=[ ");
                        for (double t : times) {
                            fprintf(stdout, "%g ", t);
                        }
                        fprintf(stdout, "]\n");
                    }
                }
                else {
                    fprintf(stdout, "%*s",
                            2*int(path.GetPathElementCount()-1), "");
                    fprintf(stdout, "!BAD_SAMPLES\n");
                }

                // Write other fields.
                TfTokenVector tmp = data.List(id);
                std::set<TfToken> tokens(tmp.begin(), tmp.end());
                tokens.erase(SdfFieldKeys->Custom);
                tokens.erase(SdfFieldKeys->TypeName);
                tokens.erase(SdfFieldKeys->Default);
                tokens.erase(SdfFieldKeys->TimeSamples);
                const SdfSchema& schema = SdfSchema::GetInstance();
                for (const auto& field : tokens) {
                    const VtValue value = data.Get(id, field);
                    if (value != schema.GetFallback(field)) {
                        fprintf(stdout, "%*s# %s = %s\n",
                                2*int(path.GetPathElementCount()-1), "",
                                field.GetText(),
                                TfStringify(value).c_str());
                    }
                }
            }
            else {
                VtValue specifier = data.Get(id, SdfFieldKeys->Specifier);
                if (specifier.IsHolding<SdfSpecifier>()) {
                    static const char* spec[] = { "def", "over", "class" };
                    fprintf(stdout, "%s ",
                            spec[specifier.UncheckedGet<SdfSpecifier>()]);
                }
                else {
                    fprintf(stdout, "!BAD_SPEC ");
                }

                VtValue typeName = data.Get(id, SdfFieldKeys->TypeName);
                if (typeName.IsHolding<TfToken>()) {
                    fprintf(stdout, "%s ", TfStringify(typeName).c_str());
                }
                else if (!typeName.IsEmpty()) {
                    fprintf(stdout, "!BAD_TYPE ");
                }

                fprintf(stdout, "%s\n", path.GetName().c_str());
            }
        }
        return true;
    }

    virtual void Done(const SdfAbstractData&)
    {
        // Do nothing
    }
};

static void UsdAbc_PrintTimes(const char* msg, const std::set<double>& times)
{
    fprintf(stdout, "%s: [", msg);
    for (double t : times) {
        fprintf(stdout, " %f", t);
    }
    fprintf(stdout, " ]\n");
}

struct UsdAbc_AlembicTimeVisitor : public SdfAbstractDataSpecVisitor {
    virtual bool VisitSpec(const SdfAbstractData& data, 
                           const SdfAbstractDataSpecId& id)
    {
        if (id.IsProperty()) {
            UsdAbc_PrintTimes(id.GetString().c_str(),
                           data.ListTimeSamplesForPath(id));
        }
        return true;
    }

    virtual void Done(const SdfAbstractData&)
    {
        // Do nothing
    }
};

bool
UsdAbc_TestAlembic(const std::string& pathname)
{
    if (UsdAbc_AlembicDataRefPtr data = UsdAbc_AlembicData::New()) {
        if (data->Open(pathname)) {
            // Dump prims and properties.
            fprintf(stdout, "\nWrite:\n");
            UsdAbc_AlembicWriteVisitor writeVisitor;
            UsdAbc_SortedDataSpecVisitor sortWrite(&writeVisitor);
            data->VisitSpecs(&sortWrite);

/*
            // Time samples.
            fprintf(stdout, "\nTime samples:\n");
            UsdAbc_AlembicTimeVisitor timeVisitor;
            UsdAbc_SortedDataSpecVisitor sortTime(&timeVisitor);
            data->VisitSpecs(&sortTime);
            UsdAbc_PrintTimes("all", data->ListAllTimeSamples());
*/

            // Dump all time samples of a particular property.  This is
            // intended for the standard Alembic octopus file.
            SdfPath path("/octopus_low/octopus_lowShape.extent");
            SdfAbstractDataSpecId id(&path);
            std::set<double> times = data->ListTimeSamplesForPath(id);
            if (!times.empty()) {
                fprintf(stdout, "\nExtent samples:\n");
                for (double t : times) {
                    VtValue value;
                    if (data->QueryTimeSample(id, t, &value)) {
                        fprintf(stdout, "  %f: %s\n",
                                t, TfStringify(value).c_str());
                    }
                    else {
                        fprintf(stdout, "  %f: <no value>\n", t);
                    }
                }

                // Verify no samples at times not listed.
                if (times.size() > 1) {
                    double t      = floor(*times.begin());
                    double tUpper = ceil(*times.rbegin());
                    for (; t <= tUpper; t += 1.0) {
                        if (times.find(t) == times.end()) {
                            if (data->QueryTimeSample(id, t, (VtValue*)NULL)) {
                                fprintf(stdout, "  %f: <expected sample>\n", t);
                            }
                        }
                    }
                }
            }

            return true;
        }
        else {
            fprintf(stderr, "Can't open Alembic file \"%s\"\n", pathname.c_str());
        }
    }
    else {
        fprintf(stderr, "Can't create Alembic data\n");
    }
    return false;
}

bool
UsdAbc_WriteAlembic(const std::string& srcPathname, const std::string& dstPathname)
{
    SdfLayerRefPtr layer = SdfLayer::OpenAsAnonymous(srcPathname);
    if (!layer) {
        fprintf(stderr, "Can't open '%s'\n", srcPathname.c_str());
        return false;
    }

    // Write the file back out in the cwd.
    return
        SdfFileFormat::FindByExtension(".abc")->
            WriteToFile(boost::get_pointer(layer), dstPathname);
}

PXR_NAMESPACE_CLOSE_SCOPE

