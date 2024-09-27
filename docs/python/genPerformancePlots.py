#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
"""
Generates multiline charts, one per platform, to display performance metrics
over releases on a fixed set of assets. Input yaml files must
have filenames matching the pattern <release>_<platform>_<asset>.yaml.
e.g. 24.11_linux_shaderball.yaml
"""
import argparse
import matplotlib.pyplot as plt
import os
import re
import yaml


# The naming scheme should be <release>_<platform>_<asset>.yaml
FILENAME_PATTERN = ("(?P<release>\d\d\.\d\d)_"
                    "(?P<platform>linux|windows|macos)_"
                    "(?P<asset>.*).yaml")

# Currently we display only the open and close metric over time.
METRIC_ID = "open_and_close_usdview"


def collectData(dataDir: str):
    yamls = [f for f in os.listdir(dataDir) if f.endswith(".yaml")]

    data = {"macos": {}, "windows": {}, "linux": {}}
    for yamlname in yamls:
        match = re.match(FILENAME_PATTERN, yamlname)
        if match is None:
            continue

        release = match["release"]
        platform = match["platform"]
        asset = match["asset"]
        with open(os.path.join(dataDir, yamlname)) as f:
            metrics = yaml.safe_load(f)
            time = metrics[METRIC_ID]["min"]

        if asset not in data[platform]:
            data[platform][asset] = []

        data[platform][asset].append((release, time))

    return data


def exportCharts(data, outputDir: str):
    for platform, platformData in data.items():
        fig, _ = plt.subplots()
        plt.close(fig)
        plt.close()
        plt.cla()
        plt.clf()

        for asset, assetData in platformData.items():
            releases = [d[0] for d in assetData]
            times = [d[1] for d in assetData]
            plt.plot(releases, times, label=asset, marker='o')

        plt.title(f"{platform} time to open and close usdview",
                  fontweight="bold")
        plt.xlabel("OpenUSD release")
        plt.ylabel("Minimum duration of 10 iterations (seconds)")
        plt.legend()
        plt.savefig(os.path.join(outputDir, f"{platform}.svg"))


def parseArgs():
    usage = "Generate a plot to be used in public performance reports"
    parser = argparse.ArgumentParser(description=usage)
    parser.add_argument('metricsDirectory',
                        type=str,
                        help="The directory from which to read metric yaml "
                             "files. File names must have format "
                             "<release>_<platform>_<asset>.yaml where release "
                             "is in format yy.mm (e.g. 24.11), and platform "
                             "is one of [linux, windows, macos].")
    parser.add_argument('outputDirectory',
                        type=str,
                        default=".",
                        help="The directory to write performance charts "
                             "out to.")

    args = parser.parse_args()

    if not os.path.isdir(args.metricsDirectory):
        raise FileNotFoundError(f"Directory {args.metricsDirectory} not found")

    if not os.path.isdir(args.outputDirectory):
        raise FileNotFoundError(f"Directory {args.outputDirectory} not found")

    return args


def main():
    args = parseArgs()
    data = collectData(args.metricsDirectory)
    exportCharts(data, args.outputDirectory)


if __name__ == "__main__":
    main()
