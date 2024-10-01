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
"""
import argparse
import functools
import os
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


def parseOutput(output, parseFn):
    """
    Invokes `parseFn` and converts the returned tuples of
    (success, metric_identifier, time) to a dictionary of
    (metric_identifier, time).
    """
    metrics = {}
    for line in output.splitlines():
        res = parseFn(line)
        if res[0]:
            metrics[res[1]] = res[2]

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


def measurePerformance(assetPath):
    """
    Run usdview to produce native timing information.
    """
    command = f"usdview --quitAfterStartup --timing {assetPath}"
    try:
        result = subprocess.run(command,
                                shell=True,
                                capture_output=True,
                                check=True)
    except subprocess.CalledProcessError as e:
        print("ERROR", e.stderr.decode(), file=sys.stderr)
        raise

    output = result.stdout.decode()
    return parseOutput(output, parseTiming)


def measureTestusdviewPerf(assetPath, testusdviewMetrics):
    """
    Run timing scripts for metrics registered in `testusdviewMetrics`.
    """
    metrics = {}
    for script, metricExpressions in testusdviewMetrics.items():
        command = ("testusdview --norender --testScript "
                   f"{script} {assetPath}")
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
        currMetrics = parseOutput(output, parseFn)
        metrics.update(currMetrics)

    return metrics


def export(metricsList, outputPath, aggregations):
    """
    Write `metrics` to the given `outputPath`. If zero aggregations,
    the reported yaml has form { name : list of times }. If one aggregation,
    the reported yaml has form { name_<agg> : aggregated time }. If multiple,
    the reported yaml has form { name : { agg1 : time, agg2 : time, ... }}
    """
    # Transpose list of metrics to dict of (metric name, list of values)
    metricsDict = {}
    for metric in metricsList:
        for name, time in metric.items():
            if name not in metricsDict:
                metricsDict[name] = []

            metricsDict[name].append(time)

    if len(aggregations) == 0:
        resultDict = metricsDict
    elif len(aggregations) == 1:
        resultDict = {}
        agg = aggregations[0]
        for name, times in metricsDict.items():
            aggName = f"{name}_{agg}"
            if agg == "min":
                resultDict[aggName] = min(times)
            elif agg == "mean":
                resultDict[aggName] = statistics.mean(times)
            elif agg == "max":
                resultDict[aggName] = max(times)
            else:
                raise ValueError(f"Internal error -- aggregation {agg}"
                                 " not implemented")
    else:
        resultDict = {}
        for name, times in metricsDict.items():
            resultDict[name] = {}
            for agg in aggregations:
                if agg == "min":
                    resultDict[name][agg] = min(times)
                elif agg == "mean":
                    resultDict[name][agg] = statistics.mean(times)
                elif agg == "max":
                    resultDict[name][agg] = max(times)
                else:
                    raise ValueError("Internal error -- aggregation "
                                     f"{agg} not implemented")

    if outputPath.endswith(".yaml"):
        with open(outputPath, "w") as f:
            yaml.dump(resultDict, f)
    else:
        raise ValueError("Internal error -- output path must be validated "
                         "at argument parse time.")

    print(f"Performance metrics have been output to {outputPath}")


def run(assetPath, testusdviewMetrics):
    """
    Collect performance metrics.
    """
    # Measure `usdview --timing` native metrics
    usdviewMetrics = measurePerformance(assetPath)

    # Measure custom `testusdview` metrics
    customMetrics = measureTestusdviewPerf(assetPath,
                                           testusdviewMetrics)

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
                             "requested, the output yaml format will be"
                             "<metric_name>: {<agg1>: <value1>, <agg2>:...}."
                             "When no aggregation is set, the output format "
                             "will be <metric_name>: [<val1>, <val2>, ...] or "
                             "one measured value for each iteration.")

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

    return args


def main():
    args = parseArgs()
    customMetrics = parseCustomMetrics(args.custom_metrics)
    warmCache(args.asset)

    metricsList = []
    for _ in range(args.iterations):
        metrics = run(args.asset, customMetrics)
        metricsList.append(metrics)

    export(metricsList, args.output, args.aggregation)


if __name__ == "__main__":
    main()
