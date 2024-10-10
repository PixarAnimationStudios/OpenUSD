#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
"""These utilities parse usdview timing output into tuples of
(success, identifier, timing).
"""
import re

make_re = lambda s: re.compile('(Time\ to\ )' + s + '(.*s)')

# `usdview --timing` metrics
METRICS = [
    ("configure_and_load_plugins",  make_re('(configure\ and\ load\ plugins:)')),
    ("open_stage", make_re('(open\ stage.*:)')),
    ("reset_prim_browser", make_re('(reset\ Prim\ Browser\ to.*:)')),
    ("bring_up_the_ui", make_re('(bring\ up\ the\ UI:)')),
    ("create_first_image", make_re('(create\ first\ image:)')),
    ("shut_down_hydra", make_re('(shut\ down\ Hydra:)')),
    ("close_stage", make_re('(close\ stage.*:)')),
    ("tear_down_the_ui", make_re('(tear\ down\ the\ UI:)')),
    ("open_and_close_usdview", make_re('(open\ and\ close\ usdview:)')),
]


def nameToMetricIdentifier(desc):
    """
    From a metric name, return a standardized identifier
    following an "underscore instead of spaces" convention.
    """
    return desc.lower().replace(" ", "_")


def parseTiming(line):
    """
    Parses tuple of (success, metric_identifier, time) from the given
    input line based on the identifier, regex combinations in METRICS.
    """
    for ident, re_expr in METRICS:
        match = re_expr.match(line)
        if match:
            groups = match.groups()
            time = float(groups[2].strip()[:-1])
            return (True, ident, time)
    return (False, None, None)


def parseTimingGeneric(metricExpressions, line):
    """
    Produces tuple of (success, metric_identifier, time) from the given
    input line based on regexes provided by metricExpressions.
    """
    metrics = [
        make_re('{profile}:'.format(profile=profile))
        for profile in metricExpressions
    ]

    for e in metrics:
        match = e.match(line)
        if match:
            groups = match.groups()
            metricName = groups[1].strip()
            ident = nameToMetricIdentifier(metricName)
            time = float(groups[2].strip()[:-1])
            return (True, ident, time)

    return (False, None, None)
