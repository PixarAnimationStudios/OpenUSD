#!/pxrpythonsubst
#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
"""This script produces and exports performance metrics. Please ensure
`usdview` and `testusdview` are on your PATH or aliased.

By default, all metrics from `usdview --timing` and EXPLICIT_METRICS
(invoking `testusdview`) are measured and reported. To provide
any additional custom scripts to `testusdview` in addition to the default
EXPLICIT_METRICS, please provide the "--custom-metrics" command line argument.
See the --help documentation for more info on `--custom-metrics` format.

If there exists a file ending in `overrides.usda` in the same directory as the
given asset file, the file will be supplied as `--sessionLayer` to usdview and
testusdview invocations. This allows provision of specific variant selections,
for example. The first file found by os.listdir will be used. Ensure there is
only one file ending in `overrides.usda` in the asset directory to remove
ambiguity.
"""
import argparse
import functools
import re
import os
import shutil
import statistics
import subprocess
import sys
import yaml

from pxr.Performance import parseTimingOutput

from pxr.Performance.parseTimingOutput import (
    METRICS,
    nameToMetricIdentifier,
    parseTiming,
    parseTimingGeneric)

SCRIPT_DIR = os.path.join(
    os.path.dirname(os.path.abspath(parseTimingOutput.__file__)),
    "ExplicitMetrics")

EXPLICIT_METRICS = {
    os.path.join(SCRIPT_DIR, "stageTraversalMetric.py"): ["(traverse stage)"]
}


def parseOutput(output, parseFn, traceFile = None):
    """
    Invokes `parseFn` and converts the returned tuples of
    (success, metric_identifier, time) to a dictionary of
    (metric_identifier, (time, traceFile)) if traces are being collected,
    (metric_identifier, (time, None)) otherwise.
    """
    metrics = {}
    for line in output.splitlines():
        res = parseFn(line)
        if res[0]:
            metrics[res[1]] = (res[2], traceFile)

    return metrics


def warmCache(assetPath):
    """
    Open usdview and load the asset to warm the system and
    filesystem cache.
    """
    command = f"usdview --quitAfterStartup --timing {assetPath}"
    try:
        subprocess.run(command,
                       shell=True,
                       capture_output=True,
                       check=True)
    except subprocess.CalledProcessError as e:
        print(e.stderr, file=sys.stderr)
        raise


def measurePerformance(assetPath, traceDir, iteration, sessionLayer):
    """
    Run usdview to produce native timing information.
    """
    traceArgs = ""
    traceFile = None
    if traceDir is not None:
        traceFile = os.path.join(traceDir, f"usdview_{iteration}.trace")
        traceArgs = f"--traceToFile {traceFile} --traceFormat trace"

    if sessionLayer:
        sessionLayer = f"--sessionLayer {sessionLayer}"
    else:
        sessionLayer = ""

    command = (f"usdview --quitAfterStartup --timing {sessionLayer} "
               f"{assetPath} {traceArgs}")
    try:
        result = subprocess.run(command,
                                shell=True,
                                capture_output=True,
                                check=True)
    except subprocess.CalledProcessError as e:
        print("ERROR", e.stderr.decode(), file=sys.stderr)
        raise

    output = result.stdout.decode()
    return parseOutput(output, parseTiming, traceFile)


def measureTestusdviewPerf(assetPath,
                           testusdviewMetrics,
                           traceDir,
                           iteration,
                           sessionLayer):
    """
    Run timing scripts for metrics registered in `testusdviewMetrics`.
    """
    if sessionLayer:
        sessionLayer = f"--sessionLayer {sessionLayer}"
    else:
        sessionLayer = ""

    metrics = {}
    for script, metricExpressions in testusdviewMetrics.items():
        traceArgs = ""
        traceFile = None
        if traceDir is not None:
            scriptRep = re.sub(r'\W+', '_', os.path.basename(script))
            traceFile = os.path.join(traceDir,
                f"testusdview_{scriptRep}_{iteration}.trace")
            traceArgs = f"--traceToFile {traceFile} --traceFormat trace"

        command = (f"testusdview --norender {traceArgs} --testScript "
                   f"{script} {sessionLayer} {assetPath}")
        try:
            result = subprocess.run(command,
                                    shell=True,
                                    capture_output=True,
                                    check=True)
        except subprocess.CalledProcessError as e:
            print("ERROR:", e.stderr.decode(), file=sys.stderr)
            raise

        output = result.stdout.decode()
        parseFn = functools.partial(parseTimingGeneric, metricExpressions)
        currMetrics = parseOutput(output, parseFn, traceFile)
        metrics.update(currMetrics)

    return metrics


def export(metricsList, outputPath, aggregations, traceDir):
    """
    Write `metrics` to the given `outputPath`. If zero aggregations,
    the reported yaml has form { name : list of times }. If one aggregation,
    the reported yaml has form { name_<agg> : aggregated time }. If multiple,
    the reported yaml has form { name : { agg1 : time, agg2 : time, ... }}.

    In addition, if traces have been requested, copy source trace files for
    min/max metrics to <metric name>_<aggregation>.trace
    """
    # All original trace files from all iterations, to be deleted.
    pendingDeletes = set()

    # Transpose list of metrics to dict of (metric name, list of values)
    metricsDict = {}
    for metric in metricsList:
        for name, (time, traceFile) in metric.items():
            if name not in metricsDict:
                metricsDict[name] = []

            if traceFile is not None:
                pendingDeletes.add(traceFile)

            metricsDict[name].append((time, traceFile))

    # Trace file source to destination filenames, relevant only when
    # traceDir is not None
    pendingCopies = {}

    # Dict to output to metrics.yaml
    resultDict = {}

    if len(aggregations) == 0:
        for name, timeTuples in metricsDict.items():
            resultDict[name] = [t[0] for t in timeTuples]
    else:
        for name, timeTuples in metricsDict.items():
            resultDict[name] = {}
            for agg in aggregations:
                if agg == "min":
                    time, traceFile = min(timeTuples)
                    pendingCopies[f"{name}_{agg}.trace"] = traceFile
                elif agg == "mean":
                    times = [t[0] for t in timeTuples]
                    time = statistics.mean(times)
                elif agg == "max":
                    time, traceFile = max(timeTuples)
                    pendingCopies[f"{name}_{agg}.trace"] = traceFile
                else:
                    raise ValueError("Internal error -- aggregation "
                                     f"{agg} not implemented")

                resultDict[name][agg] = time

    # Collapse { name : { agg : time }} to { name_agg : time } when
    # only one aggregation is given.
    if len(aggregations) == 1:
        collapsedDict = {}
        for name, aggDict in resultDict.items():
            for agg, time in aggDict.items():
                aggName = f"{name}_{agg}"
                collapsedDict[aggName] = time

        resultDict = collapsedDict

    if outputPath.endswith(".yaml"):
        with open(outputPath, "w") as f:
            yaml.dump(resultDict, f)
    else:
        raise ValueError("Internal error -- output path must be validated "
                         "at argument parse time.")

    print(f"Performance metrics have been output to {outputPath}")

    # If traces are requested, any min/max metric's associated trace file
    # will be copied to a file that looks like
    # <metric name>_<aggregation>.trace
    if traceDir is not None:
        for filename, src in pendingCopies.items():
            dest = os.path.join(traceDir, filename)
            shutil.copyfile(src, dest)

    # Delete original per-iteration trace files
    for trace in pendingDeletes:
        os.remove(trace)


def run(assetPath, testusdviewMetrics, traceDir, iteration):
    """
    Collect performance metrics.
    """
    # Supply session layer overrides, if found
    assetDir = os.path.dirname(assetPath)
    sessionLayer = None
    for fname in os.listdir(assetDir):
        if fname.endswith("overrides.usda"):
            sessionLayer = os.path.join(assetDir, fname)
            break

    # Measure `usdview --timing` native metrics
    usdviewMetrics = measurePerformance(assetPath,
                                        traceDir,
                                        iteration,
                                        sessionLayer)

    # Measure custom `testusdview` metrics
    customMetrics = measureTestusdviewPerf(assetPath,
                                           testusdviewMetrics,
                                           traceDir,
                                           iteration,
                                           sessionLayer)

    metrics = {}
    metrics.update(usdviewMetrics)
    metrics.update(customMetrics)
    return metrics


def parseCustomMetrics(customMetricInfos):
    """
    Parse any user-provided custom metric arguments for `testusdview`.
    These are run in addition to the EXPLICIT_METRICS that are run by
    default. If none are provided, return just the EXPLICIT_METRICS
    for `testusdview`.

    Raises an exception if duplicate metric names are discovered.
    """
    customMetrics = EXPLICIT_METRICS

    if customMetricInfos:
        for metricInfo in customMetricInfos:
            try:
                scriptPath, customMetricNames = metricInfo.split(":")
                customMetricNames = customMetricNames[1:-1].split(",")
            except:
                print(f"ERROR: custom metric {metricInfo} has invalid format",
                    file=sys.stderr)
                raise

            customMetrics[scriptPath] = customMetricNames

    # Confirm metrics identifiers from various sources are non-overlapping
    customMetricIds = [nameToMetricIdentifier(name)
                       for customMetricNames in customMetrics.values()
                       for name in customMetricNames]

    ids = list(m[0] for m in METRICS) + customMetricIds
    ids = sorted(ids)
    for i in range(0, len(ids) - 1):
        if ids[i] == ids[i + 1]:
            raise ValueError(f"{ids[i]} has more than one timing "
                             "value. Please rename your custom metric.")

    return customMetrics


def parseArgs():
    parser = argparse.ArgumentParser(description="Measure and export USD"
                                                 " functional performance")
    parser.add_argument("asset",
                        type=str,
                        help="Asset file path.")
    parser.add_argument("-c", "--custom-metrics",
                        type=str,
                        nargs="*",
                        help="Additional custom metrics to run `testusdview` "
                             "timing on. "
                             "If not set, a default set of metrics is run. "
                             "This should be a whitespace-separated list of "
                             "<script>:'<metric name>,<metric name>' e.g. "
                             "stageTraversalMetric.py:'traverse stage'. Note "
                             "the <script> path is relative to the directory "
                             "from which usdmeasureperformance.py is invoked.")
    parser.add_argument("-o", "--output",
                        type=str,
                        required=True,
                        help="Required output file path for metrics data. "
                             "Must be .yaml")
    parser.add_argument("-i", "--iterations",
                        type=int,
                        default=3,
                        help="Number of iterations to run. Performance "
                             "metrics are reported as an average across all "
                             "iterations. Requires -o to be set. "
                             "By default, 3 iterations are set.")
    parser.add_argument("-a", "--aggregation", type=str,
                        choices=["min", "mean", "max"],
                        nargs="+",
                        default=[],
                        help="When multiple iterations are set, report an "
                             "aggregated statistic instead of every value. "
                             "Requires -o to be set. When one aggregation is "
                             "requested, the output yaml format "
                             "will be a key value dictionary with "
                             "<metric_name>_<aggregation> to aggregated "
                             "time value. If multiple aggregations are "
                             "requested, the output yaml format will be "
                             "<metric_name>: {<agg1>: <value1>, <agg2>:...}."
                             "When no aggregation is set, the output format "
                             "will be <metric_name>: [<val1>, <val2>, ...] or "
                             "one measured value for each iteration.")
    parser.add_argument("-t", "--tracedir", type=str,
                        help="Outputs a trace file for each run of usdview in "
                             "the given directory if provided and if "
                             "'aggregation' includes min or max. A trace file "
                             "for the iteration of testusdview or usdview "
                             "from which the aggregated value of each metric "
                             "was observed will be output in the form "
                             "<metric name>_<aggregation>.trace")

    args = parser.parse_args()

    # Validate arguments
    if not os.path.exists(args.asset):
        raise FileNotFoundError(f"{args.asset} not found")
    if args.output is not None and not args.output.endswith(".yaml"):
        raise ValueError(f"Export format must be yaml, {args.output} "
                         "was requested")
    if args.iterations < 1:
        raise ValueError(f"Invalid number of iterations: {args.iterations}")
    if args.aggregation and args.output is None:
        raise ValueError("Invalid arguments: -o was not set while "
                         "an aggregation was set")

    if args.aggregation and args.iterations == 1:
        print(f"WARNING: aggregation {args.aggregation} is set but "
              "iterations is 1")
    
    aggs = args.aggregation
    if args.tracedir and ("min" in aggs or "max" in aggs):
        if not os.path.exists(args.tracedir):
            os.makedirs(args.tracedir, exist_ok=True)
            print(f"Created trace output directory {args.tracedir}")

        print(f"Trace files will be output to the '{args.tracedir}' dir")
    else:
        print("Trace files will not be output, missing --tracedir ",
              "or --aggregation that includes min or max")

    return args


def main():
    args = parseArgs()
    customMetrics = parseCustomMetrics(args.custom_metrics)
    warmCache(args.asset)

    metricsList = []
    for i in range(args.iterations):
        metrics = run(args.asset, customMetrics, args.tracedir, i)
        metricsList.append(metrics)

    export(metricsList, args.output, args.aggregation, args.tracedir)


if __name__ == "__main__":
    main()
