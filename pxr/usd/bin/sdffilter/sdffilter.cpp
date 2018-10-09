//
// Copyright 2018 Pixar
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
#include "pxr/base/arch/attributes.h"
#include "pxr/base/arch/pragmas.h"
#include "pxr/base/arch/vsnprintf.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/ostreamMethods.h"
#include "pxr/base/tf/patternMatcher.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/scopeDescription.h"
// copyUtils.h pulls in boost/any.hpp via tf/diagnostic, and this winds up
// generating an erroneous maybe-uninitialized warning on certain gcc & boost
// versions, so we disable that diagnostic around this include.
ARCH_PRAGMA_PUSH
ARCH_PRAGMA_MAYBE_UNINITIALIZED
#include "pxr/usd/sdf/copyUtils.h"
ARCH_PRAGMA_POP
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/textFileFormat.h"

#include <boost/program_options.hpp>

#include <cstdio>
#include <cstdarg>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

using std::string;
using std::vector;
using std::ostream;
using std::pair;
using std::unordered_map;
using std::unordered_set;

// A file format for the human readable "pseudoLayer" output.  We use this so
// that the terse human-readable output we produce is not a valid layer nor may
// be mistaken for one.
class SdfFilterPseudoFileFormat : public SdfTextFileFormat
{
private:
    SDF_FILE_FORMAT_FACTORY_ACCESS;

public:
    SdfFilterPseudoFileFormat(string description="<< human readable >>")
        : SdfTextFileFormat(TfToken("pseudosdf"),
                            TfToken(description),
                            SdfTextFileFormatTokens->Target) {}
private:

    virtual ~SdfFilterPseudoFileFormat() {}
};

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(SdfFilterPseudoFileFormat, SdfTextFileFormat);
}

namespace {

// the basename of the executable.
string progName;

// print/error utilities.
void VErr(char const *fmt, va_list ap) {
    fprintf(stderr, "%s: Error - %s\n", progName.c_str(),
            ArchVStringPrintf(fmt, ap).c_str());
}

void VPrint(char const *fmt, va_list ap) { vprintf(fmt, ap); }

void Err(char const *fmt, ...) ARCH_PRINTF_FUNCTION(1, 2);
void Err(char const *fmt, ...) {
    va_list ap; va_start(ap, fmt); VErr(fmt, ap); va_end(ap);
}

void ErrExit(char const *fmt, ...) ARCH_PRINTF_FUNCTION(1, 2);
void ErrExit(char const *fmt, ...) {
    va_list ap; va_start(ap, fmt); VErr(fmt, ap); va_end(ap);
    exit(1);
}

void Print(char const *fmt, ...) ARCH_PRINTF_FUNCTION(1, 2);
void Print(char const *fmt, ...) {
    va_list ap; va_start(ap, fmt); VPrint(fmt, ap); va_end(ap);
}

// The sorting key for 'outline' output.
enum SortKey {
    SortByPath,
    SortByField
};

std::ostream &
operator<<(std::ostream &os, SortKey key)
{
    if (key == SortByPath) {
        return os << "path";
    }
    else if (key == SortByField) {
        return os << "field";
    }
    TF_CODING_ERROR("Invalid value for SortKey (%d)",
                    static_cast<int>(key));
    return os << "invalid";
}

std::istream &
operator>>(std::istream &is, SortKey &key)
{
    std::string txt;
    is >> txt;
    if (txt == "path") {
        key = SortByPath;
    }
    else if (txt == "field") {
        key = SortByField;
    }
    else {
        is.setstate(std::ios_base::failbit);
    }
    return is;
}

// An enum representing the type of output to produce.
enum OutputType {
    OutputValidity,     // only check file validity by reading all values.
    OutputSummary,      // report a brief summary with file statistics.
    OutputOutline,      // report as an outilne, either by path or by field.
    OutputPseudoLayer,  // report as human readable text, as close to a
                        // valid layer as possible
    OutputLayer         // produce a valid layer as output.
};

std::ostream &
operator<<(std::ostream &os, OutputType outputType)
{
    if (outputType == OutputValidity) {
        return os << "validity";
    }
    else if (outputType == OutputSummary) {
        return os << "summary";
    }
    else if (outputType == OutputOutline) {
        return os << "outline";
    }
    else if (outputType == OutputPseudoLayer) {
        return os << "pseudoLayer";
    }
    else if (outputType == OutputLayer) {
        return os << "layer";
    }
    TF_CODING_ERROR("Invalid value for OutputType (%d)",
                    static_cast<int>(outputType));
    return os << "invalid";
}

std::istream &
operator>>(std::istream &is, OutputType &outputType)
{
    std::string txt;
    is >> txt;
    if (txt == "validity") {
        outputType = OutputValidity;
    }
    else if (txt == "summary") {
        outputType = OutputSummary;
    }
    else if (txt == "outline") {
        outputType = OutputOutline;
    }
    else if (txt == "pseudoLayer") {
        outputType = OutputPseudoLayer;
    }
    else if (txt == "layer") {
        outputType = OutputLayer;
    }
    else {
        is.setstate(std::ios_base::failbit);
    }
    return is;
}

// We use this structure to represent all the parameters for reporting.  We fill
// this using command-line args.
struct ReportParams
{
    std::shared_ptr<TfPatternMatcher> pathMatcher;
    std::shared_ptr<TfPatternMatcher> fieldMatcher;

    OutputType outputType;
    std::string outputFile;
    std::string outputFormat;
    
    std::vector<double> literalTimes;
    std::vector<std::pair<double, double>> timeRanges;
    double timeTolerance;

    SortKey sortKey = SortByPath;
    int64_t arraySizeLimit = -1;
    int64_t timeSamplesSizeLimit = -1;    
    bool showValues = true;
};

// Summary statistics for 'summary' output.
struct SummaryStats
{
    size_t numSpecs = 0;
    size_t numPrimSpecs = 0;
    size_t numPropertySpecs = 0;
    size_t numFields = 0;
    size_t numSampleTimes = 0;
};

// Parse times and time ranges in timeSpecs, throw an exception if something
// goes wrong.
void
ParseTimes(vector<string> const &timeSpecs,
           vector<double> *literalTimes,
           vector<pair<double, double>> *timeRanges)
{
    for (auto const &spec: timeSpecs) {
        try {
            if (TfStringContains(spec, "..")) {
                auto elts = TfStringSplit(spec, "..");
                if (elts.size() != 2) {
                    throw std::invalid_argument(
                        TfStringPrintf("invalid time syntax '%s'",
                                       spec.c_str()));
                }
                timeRanges->emplace_back(boost::lexical_cast<double>(elts[0]),
                                         boost::lexical_cast<double>(elts[1]));
            } else {
                literalTimes->emplace_back(boost::lexical_cast<double>(spec));
            }
        } catch (boost::bad_lexical_cast const &) {
            throw std::invalid_argument(
                TfStringPrintf("invalid time syntax '%s'", spec.c_str()));
        }
    }
    sort(literalTimes->begin(), literalTimes->end());
    literalTimes->erase(unique(literalTimes->begin(), literalTimes->end()),
                        literalTimes->end());
    sort(timeRanges->begin(), timeRanges->end());
    timeRanges->erase(unique(timeRanges->begin(), timeRanges->end()),
                      timeRanges->end());
}

// Find all the paths in layer that match, or all paths if matcher is null.
std::vector<SdfPath>
CollectMatchingSpecPaths(SdfLayerHandle const &layer,
                         TfPatternMatcher const *matcher)
{
    std::vector<SdfPath> result;
    layer->Traverse(SdfPath::AbsoluteRootPath(),
                    [&result, &matcher](SdfPath const &path) {
                        if (!matcher || matcher->Match(path.GetString()))
                            result.push_back(path);
                    });
    return result;
}

// Find all the fields for the given path that match, or all fields if matcher
// is null.
std::vector<TfToken>
CollectMatchingFields(SdfLayerHandle const &layer,
                      SdfPath const &path,
                      TfPatternMatcher const *matcher)
{
    std::vector<TfToken> fields = layer->ListFields(path);
    fields.erase(remove_if(fields.begin(), fields.end(),
                           [&matcher](TfToken const &f) {
                               return matcher && !matcher->Match(f.GetString());
                           }),
                 fields.end());
    return fields;
}

// Closeness check with relative tolerance.
bool IsClose(double a, double b, double tol)
{
    double absDiff = fabs(a-b);
    return absDiff <= fabs(tol*a) || absDiff < fabs(tol*b);
}

// Get a suitable value for the report specified by p.  In particular, for
// non-layer output, make a value that shows only array type & size for large
// arrays.
VtValue
GetReportValue(VtValue const &value, ReportParams const &p)
{
    if (p.outputType != OutputLayer &&
        p.arraySizeLimit >= 0 &&
        value.IsArrayValued() &&
        value.GetArraySize() > static_cast<uint64_t>(p.arraySizeLimit)) {
        return VtValue(
            SdfHumanReadableValue(
                TfStringPrintf(
                    "%s[%zu]",
                    ArchGetDemangled(value.GetElementTypeid()).c_str(),
                    value.GetArraySize())));
    }
    return value;
}

// Get a suitable value for timeSamples for the report specified by p.  In
// particular, for non-layer output, make a value that shows number of samples
// and their time range.
VtValue
GetReportTimeSamplesValue(SdfLayerHandle const &layer,
                          SdfPath const &path, ReportParams const &p)
{
    bool takeAllTimes = p.literalTimes.empty() && p.timeRanges.empty();
    auto times = layer->ListTimeSamplesForPath(path);
    std::vector<double> selectedTimes;
    selectedTimes.reserve(times.size());

    if (takeAllTimes) {
        selectedTimes.assign(times.begin(), times.end());
    }
    else {
        for (auto time: times) {
            // Check literalTimes.
            auto rng = equal_range(
                p.literalTimes.begin(), p.literalTimes.end(), time,
                [&p](double a, double b)  {
                    return IsClose(a, b, p.timeTolerance) ? false : a < b;
                });
            if (rng.first != rng.second)
                selectedTimes.push_back(time);

            // Check ranges.
            for (auto const &range: p.timeRanges) {
                if (range.first <= time && time <= range.second)
                    selectedTimes.push_back(time);
            }
        }
    }

    if (selectedTimes.empty())
        return VtValue();

    SdfTimeSampleMap result;
    
    VtValue val;
    if (p.outputType != OutputLayer &&
        p.timeSamplesSizeLimit >= 0 &&
        selectedTimes.size() > static_cast<uint64_t>(p.timeSamplesSizeLimit)) {
        return VtValue(
            SdfHumanReadableValue(
                TfStringPrintf("%zu samples in [%s, %s]",
                               times.size(),
                               TfStringify(*times.begin()).c_str(),
                               TfStringify(*(--times.end())).c_str())
                )
            );
    }
    else {
        for (auto time: selectedTimes) {
            TF_VERIFY(layer->QueryTimeSample(path, time, &val));
            result[time] = GetReportValue(val, p);
        }
    }
    return VtValue(result);
}

// Get a suitable value for the report specified by p.  In particular, for
// non-layer output, make a value that shows only array type & size for large
// arrays or number of time samples and time range for large timeSamples.
VtValue
GetReportFieldValue(SdfLayerHandle const &layer,
                           SdfPath const &path, TfToken const &field,
                           ReportParams const &p)
{
    VtValue result;
    // Handle timeSamples specially:
    if (field == SdfFieldKeys->TimeSamples) {
        result = GetReportTimeSamplesValue(layer, path, p);
    } else {
        TF_VERIFY(layer->HasField(path, field, &result));
        result = GetReportValue(result, p);
    }
    return result;
}

// Produce the 'outline' output report by path.
void
GetReportByPath(SdfLayerHandle const &layer,
                ReportParams const &p,
                std::vector<std::string> &report)
{
    std::vector<SdfPath> paths =
        CollectMatchingSpecPaths(layer, p.pathMatcher.get());
    sort(paths.begin(), paths.end());
    for (auto const &path: paths) {
        SdfSpecType specType = layer->GetSpecType(path);
        report.push_back(
            TfStringPrintf(
                "<%s> : %s", 
                path.GetText(), TfStringify(specType).c_str()));

        std::vector<TfToken> fields =
            CollectMatchingFields(layer, path, p.fieldMatcher.get());
        if (fields.empty())
            continue;
        for (auto const &field: fields) {
            if (p.showValues) {
                report.push_back(
                    TfStringPrintf(
                        "  %s: %s", field.GetText(),
                        TfStringify(GetReportFieldValue(
                                        layer, path, field, p)).c_str())
                    );
            } else {
                report.push_back(TfStringPrintf("  %s", field.GetText()));
            }
        }
    }
}

// Produce the 'outline' output report by field.
void
GetReportByField(SdfLayerHandle const &layer,
                 ReportParams const &p,
                 std::vector<std::string> &report)
{
    std::vector<SdfPath> paths =
        CollectMatchingSpecPaths(layer, p.pathMatcher.get());
    std::unordered_map<
        std::string, std::vector<std::string>> pathsByFieldString;
    std::unordered_set<std::string> allFieldStrings;
    sort(paths.begin(), paths.end());
    for (auto const &path: paths) {
        std::vector<TfToken> fields =
            CollectMatchingFields(layer, path, p.fieldMatcher.get());
        if (fields.empty())
            continue;
        for (auto const &field: fields) {
            std::string fieldString;
            if (p.showValues) {
                fieldString = TfStringPrintf(
                    "%s: %s", field.GetText(),
                    TfStringify(GetReportFieldValue(
                                    layer, path, field, p)).c_str());
            } else {
                fieldString = TfStringPrintf("%s", field.GetText());
            }
            pathsByFieldString[fieldString].push_back(
                TfStringPrintf("  <%s>", path.GetText()));
            allFieldStrings.insert(fieldString);
        }
    }
    std::vector<std::string>
        fsvec(allFieldStrings.begin(), allFieldStrings.end());
    sort(fsvec.begin(), fsvec.end());

    for (auto const &fs: fsvec) {
        report.push_back(fs);
        auto const &ps = pathsByFieldString[fs];
        report.insert(report.end(), ps.begin(), ps.end());
    }
}

// Compute and return the summary statistics for the given layer.
SummaryStats
GetSummaryStats(SdfLayerHandle const &layer)
{
    SummaryStats stats;
    layer->Traverse(
        SdfPath::AbsoluteRootPath(), [&stats, &layer](SdfPath const &path) {
            ++stats.numSpecs;
            stats.numPrimSpecs += path.IsPrimPath();
            stats.numPropertySpecs += path.IsPropertyPath();
            stats.numFields += layer->ListFields(path).size();
        });
    stats.numSampleTimes = layer->ListAllTimeSamples().size();
    return stats;
}

// Utility function to filter a layer by the params p.  This copies fields,
// replacing large arrays and timeSamples with human readable values if
// appropriate, and skipping paths and fields that do not match the matchers in
// p.
void
FilterLayer(SdfLayerHandle const &inLayer,
            SdfLayerHandle const &outLayer,
            ReportParams const &p)
{
    namespace ph = std::placeholders;
    auto copyValueFn = [&p](
        SdfSpecType specType, TfToken const &field,
        SdfLayerHandle const &srcLayer, const SdfPath& srcPath, bool fieldInSrc,
        const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool fieldInDst,
        boost::optional<VtValue> *valueToCopy) {

        if (!p.fieldMatcher || p.fieldMatcher->Match(field.GetString())) {
            *valueToCopy = GetReportFieldValue(
                srcLayer, srcPath, field, p);
            return !(*valueToCopy)->IsEmpty();
        }
        else {
            return false;
        }
    };

    vector<SdfPath> paths =
        CollectMatchingSpecPaths(inLayer, p.pathMatcher.get());
    for (auto const &path: paths) {
        if (path == SdfPath::AbsoluteRootPath() ||
            path.IsPrimOrPrimVariantSelectionPath()) {
            SdfPrimSpecHandle outPrim = SdfCreatePrimInLayer(outLayer, path);
            SdfCopySpec(inLayer, path, outLayer, path,
                        copyValueFn,
                        std::bind(SdfShouldCopyChildren,
                                  std::cref(path), std::cref(path),
                                  ph::_1, ph::_2, ph::_3, ph::_4, ph::_5,
                                  ph::_6, ph::_7, ph::_8, ph::_9));
        }
    }
}

// Attempt to validate a layer by reading all field values from all paths.
void
Validate(SdfLayerHandle const &layer, ReportParams const &p,
         string &report)
{
    TfErrorMark m;
    TF_DESCRIBE_SCOPE("Collecting paths in @%s@",
                      layer->GetIdentifier().c_str());
    vector<SdfPath> paths;
    layer->Traverse(SdfPath::AbsoluteRootPath(),
                    [&paths, layer](SdfPath const &path) {
                        TF_DESCRIBE_SCOPE(
                            "Collecting path <%s> in @%s@",
                            path.GetText(), layer->GetIdentifier().c_str());
                        paths.push_back(path);
                    });
    sort(paths.begin(), paths.end());
    for (auto const &path: paths) {
        TF_DESCRIBE_SCOPE("Collecting fields for <%s> in @%s@",
                     path.GetText(), layer->GetIdentifier().c_str());
        vector<TfToken> fields = layer->ListFields(path);
        if (fields.empty())
            continue;
        for (auto const &field: fields) {
            VtValue value;
            if (field == SdfFieldKeys->TimeSamples) {
                // Pull each sample value individually.
                TF_DESCRIBE_SCOPE(
                    "Getting sample times for '%s' on <%s> in @%s@",
                    field.GetText(), path.GetText(),
                    layer->GetIdentifier().c_str());
                auto times = layer->ListTimeSamplesForPath(path);

                for (auto time: times) {
                    TF_DESCRIBE_SCOPE("Getting sample value at time "
                                      "%f for '%s' on <%s> in @%s@",
                                      time, field.GetText(), path.GetText(),
                                      layer->GetIdentifier().c_str());
                    layer->QueryTimeSample(path, time, &value);
                }
            } else {
                // Just pull value.
                TF_DESCRIBE_SCOPE("Getting value for '%s' on <%s> in @%s@",
                                  field.GetText(), path.GetText(),
                                  layer->GetIdentifier().c_str());
                layer->HasField(path, field, &value);
            }
        }
    }
    report += m.IsClean() ? "OK" : "ERROR";
}

// Output helper struct.  Manages fclosing the file handle, and appending output
// for multi-layer inputs.
struct OutputFile
{
    struct Closer {
        void operator()(FILE *f) const {
            if (f && f != stdout) {
                fclose(f);
            }
        }
    };
    explicit OutputFile(ReportParams const &p) {
        if (!p.outputFile.empty()) {
            if (p.outputType != OutputLayer) {
                _file.reset(fopen(p.outputFile.c_str(), "a"));
            }
        } else {
            _file.reset(stdout);
        }
    }
    void Write(string const &text) const {
        fputs(text.c_str(), _file.get());
    }
    std::unique_ptr<FILE, Closer> _file;
};

// Top level processing function; dispatches to various output implementations.
void Process(SdfLayerHandle layer, ReportParams const &p)
{
    OutputFile output(p);
    if (p.outputType == OutputValidity) {
        std::string validateText;
        Validate(layer, p, validateText);
        output.Write(
            TfStringPrintf(
                "@%s@ - %s\n",
                layer->GetIdentifier().c_str(),
                validateText.c_str()));
    }
    else if (p.outputType == OutputSummary) {
        auto stats = GetSummaryStats(layer);
        output.Write(
            TfStringPrintf(
                "@%s@\n"
                "  %zu specs, %zu prim specs, %zu property specs, %zu fields, "
                "%zu sample times\n",
                layer->GetIdentifier().c_str(),
                stats.numSpecs, stats.numPrimSpecs, stats.numPropertySpecs,
                stats.numFields, stats.numSampleTimes));
    }
    else if (p.outputType == OutputOutline) {
        vector<string> report;
        if (p.sortKey == SortByPath) {
            GetReportByPath(layer, p, report);
        }
        else if (p.sortKey == SortByField) {
            GetReportByField(layer, p, report);
        }
        TfStringPrintf("@%s@\n", layer->GetIdentifier().c_str());
        for (string const &line: report) {
            output.Write(line);
            output.Write("\n");
        }
    }
    else if (p.outputType == OutputPseudoLayer ||
             p.outputType == OutputLayer) {
        // Make the layer and copy into it, then export.
        SdfLayerRefPtr outputLayer;
        SdfFileFormatConstRefPtr fmt;
        if (p.outputType == OutputPseudoLayer) {
            fmt = TfCreateRefPtr(
                new SdfFilterPseudoFileFormat(
                    TfStringPrintf("from @%s@",
                                   layer->GetIdentifier().c_str())));
            outputLayer = SdfLayer::CreateAnonymous(".pseudosdf", fmt);
        } else {
            SdfLayer::FileFormatArguments formatArgs;
            if (!p.outputFormat.empty()) {
                formatArgs["format"] = p.outputFormat;
            }
            outputLayer = !p.outputFile.empty() ?
                SdfLayer::CreateNew(
                    p.outputFile, /*realPath=*/string(), formatArgs) :
                SdfLayer::CreateAnonymous(
                    p.outputFormat.empty() ? string() :
                    TfStringPrintf(".%s", p.outputFormat.c_str()));
        }

        // Generate the layer content.
        FilterLayer(layer, outputLayer, p);

        // If this layer is anonymous, it means we're writing to stdout.
        if (outputLayer->IsAnonymous()) {
            string txt;
            outputLayer->ExportToString(&txt);
            output.Write(txt);
        } else {
            outputLayer->Save();
        }
    }
}

} // anon

PXR_NAMESPACE_CLOSE_SCOPE

int
main(int argc, char const *argv[])
{
    namespace po = boost::program_options;
    PXR_NAMESPACE_USING_DIRECTIVE

    progName = TfGetBaseName(argv[0]);

    string pathRegex = ".*", fieldRegex = ".*";
    OutputType outputType = OutputOutline;
    string outputFile, outputFormat;
    vector<string> timeSpecs;
    vector<string> inputFiles;
    vector<double> literalTimes;
    vector<pair<double, double>> timeRanges;
    double timeTolerance = 1.25e-4; // ugh -- chosen to print well in help.
    SortKey sortKey = SortByPath;
    int64_t arraySizeLimit = -2;
    int64_t timeSamplesSizeLimit = -2;
    bool noValues = false;

    po::options_description argOpts("Options");
    argOpts.add_options()
        ("help,h", "Show help message.")
        ("path,p", po::value<string>(&pathRegex)->value_name("regex"),
         "Report only paths matching this regex.  For 'layer' and "
         "'pseudoLayer' output types, include all descendants of matching "
         "paths.")
        ("field,f", po::value<string>(&fieldRegex)->value_name("regex"),
         "Report only fields matching this regex.")
        ("time,t", po::value<vector<string>>(&timeSpecs)->
         multitoken()->value_name("n or ff..lf"),
         "Report only these times or time ranges for 'timeSamples' fields.")
        ("timeTolerance", po::value<double>(&timeTolerance)->
         default_value(timeTolerance)->value_name("tol"),
         "Report times that are close to those requested within this "
         "relative tolerance.")
        ("arraySizeLimit", po::value<int64_t>(&arraySizeLimit)->value_name("N"),
         "Truncate arrays with more than this many elements.  If -1, do not "
         "truncate arrays.  Default: 0 for 'outline' output, 8 for "
         "'pseudoLayer' output, and -1 for 'layer' output.")
        ("timeSamplesSizeLimit",
         po::value<int64_t>(&timeSamplesSizeLimit)->value_name("N"),
         "Truncate timeSamples with more than this many values.  If -1, do not "
         "truncate timeSamples.  Default: 0 for 'outline' output, 8 for "
         "'pseudoLayer' output, and -1 for 'layer' output.  Truncation "
         "performed after initial filtering by --time arguments.")
        ("out,o",
         po::value<string>(&outputFile)->default_value(std::string())->
         value_name("outputFile"),
         "Direct output to this file.  Use the "
         "'outputFormat' for finer control over the underlying format for "
         "output formats that are not uniquely determined by file extension.")
        ("outputType",
         po::value<OutputType>(&outputType)->default_value(outputType)->
         value_name("validity|summary|outline|pseudoLayer|layer"),
         "Specify output format; 'summary' reports overall statistics, "
         "'outline' is a flat text report of paths and fields, "
         "'pseudoLayer' is similar to the sdf file format but with truncated "
         "array values and timeSamples for human readability, and 'layer' is "
         "true layer output, with the format controlled by the 'outputFile' "
         "and 'outputFormat' arguments.")
        ("outputFormat",
         po::value<string>(&outputFormat)->value_name("format"),
         "Supply this as the 'format' entry of SdfFileFormatArguments for "
         "'layer' output to a file.  Requires both 'layer' output and a "
         "specified 'outputFile'.")
        ("sortBy", po::value<SortKey>(&sortKey)->default_value(sortKey)->
         value_name("path|field"),
         "Group 'outline' output by either path or field.  Ignored for other "
         "output types.")
        ("noValues", po::bool_switch(&noValues),
         "Do not report field values for 'outline' output.  Ignored for other "
         "output types.")
        ;

    po::options_description inputFile("Input");
    inputFile.add_options()
        ("input-file", po::value<vector<string>>(&inputFiles), "input files");

    po::options_description allOpts;
    allOpts.add(argOpts).add(inputFile);

    po::variables_map vm;
    try {
        po::positional_options_description p;
        p.add("input-file", -1);
        po::store(po::command_line_parser(argc, argv).
                  options(allOpts).positional(p).run(), vm);
        po::notify(vm);
        ParseTimes(timeSpecs, &literalTimes, &timeRanges);
    } catch (std::exception const &e) {
        ErrExit("%s", e.what());
    }

    if (vm.count("help") || inputFiles.empty()) {
        fprintf(stderr, "Usage: %s [options] <input files>\n",
                progName.c_str());
        fprintf(stderr, "%s\n", TfStringify(argOpts).c_str());
        exit(1);
    }

    std::shared_ptr<TfPatternMatcher> pathMatcher(
        pathRegex != ".*" ? new TfPatternMatcher(pathRegex) : nullptr);
    if (pathMatcher && !pathMatcher->IsValid()) {
        ErrExit("path regex '%s' : %s", pathRegex.c_str(),
                pathMatcher->GetInvalidReason().c_str());
    }

    std::shared_ptr<TfPatternMatcher> fieldMatcher(
        fieldRegex != ".*" ? new TfPatternMatcher(fieldRegex) : nullptr);
    if (fieldMatcher && !fieldMatcher->IsValid()) {
        ErrExit("field regex '%s' : %s", fieldRegex.c_str(),
                fieldMatcher->GetInvalidReason().c_str());
    }

    // If --out was specified, it must either not exist or must be writable.  If
    // the output type is 'layer', then the extension must correspond to a known
    // Sdf file format and we must have exactly one input file.  If the output
    // type is not 'layer', then the extension must not correspond to a known
    // Sdf file format.
    if (!outputFile.empty()) {
        if (TfIsFile(outputFile) && !TfIsWritable(outputFile)) {
            ErrExit("no write permission for existing output file '%s'",
                    outputFile.c_str());
        }
        // Using --out With 'layer' outputType there must be exactly one input
        // file, and the output file must have a known Sdf file format.
        if (outputType == OutputLayer) {
            if (inputFiles.size() > 1) {
                ErrExit("must supply exactly one input file with "
                        "'--outputType layer'");
            }
            if (!SdfFileFormat::FindByExtension(
                    TfStringGetSuffix(outputFile))) {
                ErrExit("no known Sdf file format for output file '%s'",
                        outputFile.c_str());
            }
        }
        // On the other hand, using --out with any other output type must not
        // correspond to an Sdf format.
        if (outputType != OutputLayer) {
            if (SdfFileFormat::FindByExtension(
                    TfStringGetSuffix(outputFile))) {
                ErrExit("output type '%s' does not produce content compatible "
                        "with the format for output file '%s'",
                        TfStringify(outputType).c_str(), outputFile.c_str());
            }
        }

        // Truncate the output file to start.
        FILE *f = fopen(outputFile.c_str(), "w");
        if (!f) {
            ErrExit("Failed to truncate output file '%s'", outputFile.c_str());
        }
        fclose(f);
    }

    // Set defaults for arraySizeLimit and timeSamplesSizeLimit.
    if (arraySizeLimit == -2 /* unset */) {
        arraySizeLimit =
            outputType == OutputPseudoLayer ? 8 :
            outputType == OutputLayer ? -1 : 0;
    }
    if (timeSamplesSizeLimit == -2 /* unset */) {
        timeSamplesSizeLimit =
            outputType == OutputPseudoLayer ? 8 :
            outputType == OutputLayer ? -1 : 0;
    }

    ReportParams params;
    params.pathMatcher = pathMatcher;
    params.fieldMatcher = fieldMatcher;

    params.outputType = outputType;
    params.outputFile = outputFile;
    params.outputFormat = outputFormat;

    params.literalTimes = literalTimes;
    params.timeRanges = timeRanges;
    params.timeTolerance = timeTolerance;

    params.sortKey = sortKey;
    params.arraySizeLimit = arraySizeLimit;
    params.timeSamplesSizeLimit = timeSamplesSizeLimit;
    params.showValues = !noValues;

    for (auto const &file: inputFiles) {
        TF_DESCRIBE_SCOPE("Opening layer @%s@", file.c_str());
        auto layer = SdfLayer::FindOrOpen(file);
        if (!layer) {
            Err("failed to open layer <%s>", file.c_str());
        }
        else {
            Process(layer, params);
        }
    }

    return 0;
}
