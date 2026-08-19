#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

'''
Applies authorship records to prims in a USD layer.

Each record describes one authoring step: whatever this invocation is
recording. A prim can carry several records, so each is applied under an
instance name and lives in its own property namespace:

    authorship:<instance>:softwarePackage
    authorship:<instance>:softwareVersion
    ...

Example, recording a generative step driven by an artist:

    usdApplyAuthorship scene.usda --prim /World/Bunny --instance hunyuan3d \\
        --software-package net.trellis3d.hunyuan3d --software-version 2.1 \\
        --digital-source-type trainedAlgorithmicMedia \\
        --creator "Trellis Hunyuan 3D" --creator "John Doe" \\
        --input prompt="A fluffy bunny" --input image=./refs/bunny.png \\
        --usage-terms CC-BY-SA-4.0

Then a later human cleanup pass, recorded separately since it is its own
step on the same prim:

    usdApplyAuthorship scene.usda --prim /World/Bunny --instance blender \\
        --software-package org.blender --software-version 4.2 \\
        --digital-source-type digitalCreation --creator "Jane Doe"

This records who or what authored a prim. It does not verify that information,
and it does not establish ownership. See --copyright-owner for that. It, too,
is only a claim.
'''

import argparse
import datetime
import sys
import uuid

from pxr import Sdf, Tf, Usd, UsdMedia

# Shorthands are accepted on the command line; the full URI is authored.
IPTC_PREFIX = 'http://cv.iptc.org/newscodes/digitalsourcetype/'

DIGITAL_SOURCE_TYPES = [
    'trainedAlgorithmicMedia',
    'compositeWithTrainedAlgorithmicMedia',
    'algorithmicMedia',
    'digitalCreation',
    'digitalCapture',
    'humanEdits',
    'dataDrivenMedia',
]


def ResolveDigitalSourceType(value):
    """Expands a known IPTC shorthand, passing anything else through so other
    registrars work."""
    if value in DIGITAL_SOURCE_TYPES:
        return IPTC_PREFIX + value
    return value


def ParseInput(text):
    """Parses a --input name=value pair, splitting on the first '=' only.

    Values are stored in the index-matched prompt:inputNames and
    prompt:inputValues arrays, not as 'name=value' strings.
    """
    name, separator, value = text.partition('=')
    if not separator or not name:
        raise argparse.ArgumentTypeError(
            'expected name=value, got %r' % text)
    return (name, value)


def GetArgs():
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument('layer',
                        help='The USD layer to edit.')
    parser.add_argument('-o', '--output', default=None,
                        help='Write to this layer instead of editing the input '
                             'layer in place. Relative asset paths are not '
                             're-anchored, so write beside the input.')

    target = parser.add_argument_group('target prims')
    target.add_argument('-p', '--prim', action='append', default=[],
                        metavar='PRIMPATH',
                        help='A prim to apply the record to. Repeatable.')
    target.add_argument('--default-prim', action='store_true',
                        help="Apply to the layer's defaultPrim. This is where "
                             'authorship describing the asset as a whole '
                             'belongs.')

    record = parser.add_argument_group('the record')
    record.add_argument('-i', '--instance', required=True,
                        help='Instance name for this record, used to namespace '
                             'it on the prim. Records from separate authoring '
                             'steps need distinct names so they do not clobber '
                             'each other.')
    record.add_argument('--software-package', default=None,
                        help='Identifier for the tool or system that wrote the '
                             'data, e.g. net.trellis3d.hunyuan3d. Reverse '
                             'domain style is recommended but not required.')
    record.add_argument('--software-version', default=None,
                        help='Version of the softwarePackage, e.g. 2.1.')
    record.add_argument('--digital-source-type', default=None,
                        help='How this step produced its contribution. IPTC '
                             'shorthands are expanded to full URIs; known ones '
                             'are: ' + ', '.join(DIGITAL_SOURCE_TYPES))
    record.add_argument('--creator', action='append', default=[],
                        help='Human-readable credit for a person, studio or '
                             'tool. Repeatable.')
    record.add_argument('--description', default=None,
                        help='Free-form notes about how this was made.')
    record.add_argument('--input', action='append', default=[],
                        type=ParseInput, metavar='NAME=VALUE',
                        help='An input that shaped this step, e.g. '
                             'prompt="A fluffy bunny" or seed=1234567. '
                             'Repeatable. Stored as index-matched '
                             'prompt:inputNames and prompt:inputValues '
                             'arrays.')
    record.add_argument('--usage-terms', default=None,
                        help='License or usage terms. An SPDX identifier such '
                             'as CC-BY-SA-4.0 is preferred, else a URL, else '
                             'the full text.')
    record.add_argument('--copyright-owner', action='append', default=[],
                        help='An entity holding copyright, which is often not '
                             'the author. Repeatable.')
    record.add_argument('--contact', action='append', default=[],
                        help='Contact for questions about this asset. '
                             'Repeatable.')

    identity = parser.add_argument_group('timestamp and identifier')
    createdGroup = identity.add_mutually_exclusive_group()
    createdGroup.add_argument('--created', default=None,
                              help='ISO 8601 timestamp with timezone for this '
                                   'step. Defaults to now.')
    createdGroup.add_argument('--no-created', action='store_true',
                              help='Do not author a created timestamp.')
    idGroup = identity.add_mutually_exclusive_group()
    idGroup.add_argument('--instance-id', default=None,
                         help="Identifier for this run's output. Defaults to "
                              'a UUID4.')
    idGroup.add_argument('--no-instance-id', action='store_true',
                         help='Do not author an instanceID.')

    args = parser.parse_args()

    if not args.prim and not args.default_prim:
        parser.error('specify at least one --prim, or --default-prim')

    return args


def GetTargetPrims(stage, args):
    """Resolves the requested prim paths, erroring on any that do not exist."""
    prims = []

    if args.default_prim:
        prim = stage.GetDefaultPrim()
        if not prim:
            raise RuntimeError(
                '--default-prim given but the layer has no valid defaultPrim')
        prims.append(prim)

    for path in args.prim:
        if not Sdf.Path.IsValidPathString(path):
            raise RuntimeError('not a valid prim path: %s' % path)
        prim = stage.GetPrimAtPath(path)
        if not prim:
            raise RuntimeError('no prim at path: %s' % path)
        prims.append(prim)

    return prims


def ApplyRecord(prim, args, created, instanceId):
    """Applies one record, authoring only the fields that were supplied."""
    # Apply() does not validate the instance name, so ask first: a name
    # colliding with one of the schema's field names is not allowed.
    canApply, whyNot = prim.CanApplyAPI(UsdMedia.AuthorshipAPI, args.instance)
    if not canApply:
        raise RuntimeError('cannot apply AuthorshipAPI:%s to %s: %s'
                           % (args.instance, prim.GetPath(), whyNot))

    api = UsdMedia.AuthorshipAPI.Apply(prim, args.instance)
    if not api:
        raise RuntimeError(
            'could not apply AuthorshipAPI:%s to %s'
            % (args.instance, prim.GetPath()))

    if args.software_package:
        api.CreateSoftwarePackageAttr(args.software_package)
    if args.software_version:
        api.CreateSoftwareVersionAttr(args.software_version)
    if args.digital_source_type:
        api.CreateDigitalSourceTypeAttr(
            ResolveDigitalSourceType(args.digital_source_type))
    if args.creator:
        api.CreateCreatorAttr(args.creator)
    if args.description:
        api.CreateDescriptionAttr(args.description)
    if args.input:
        # Index-matched, so always authored together and in the same layer.
        api.CreatePromptInputNamesAttr([name for name, _ in args.input])
        api.CreatePromptInputValuesAttr([value for _, value in args.input])
    if created:
        api.CreateCreatedAttr(created)
    if instanceId:
        api.CreateInstanceIDAttr(instanceId)
    if args.usage_terms:
        api.CreateUsageTermsAttr(args.usage_terms)
    if args.copyright_owner:
        api.CreateCopyrightOwnerAttr(args.copyright_owner)
    if args.contact:
        api.CreateContactAttr(args.contact)

    return api


def main():
    args = GetArgs()

    try:
        stage = Usd.Stage.Open(args.layer)
        prims = GetTargetPrims(stage, args)
    except (Tf.ErrorException, RuntimeError) as error:
        print('error: %s' % error, file=sys.stderr)
        return 1

    created = None
    if not args.no_created:
        created = args.created or \
            datetime.datetime.now().astimezone().isoformat(timespec='seconds')

    # One run gets one instanceID, shared by every prim it emits.
    instanceId = None
    if not args.no_instance_id:
        instanceId = args.instance_id or str(uuid.uuid4())

    try:
        for prim in prims:
            ApplyRecord(prim, args, created, instanceId)
            print('applied AuthorshipAPI:%s to %s'
                  % (args.instance, prim.GetPath()))
    except (Tf.ErrorException, RuntimeError) as error:
        print('error: %s' % error, file=sys.stderr)
        return 1

    if args.output:
        stage.GetRootLayer().Export(args.output)
        print('wrote %s' % args.output)
    else:
        stage.GetRootLayer().Save()
        print('saved %s' % args.layer)

    return 0


if __name__ == '__main__':
    sys.exit(main())
