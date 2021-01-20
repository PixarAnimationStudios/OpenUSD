#
# Copyright 2020 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
'''Rewrite pluginfo library paths for changed library locations.

This is used for updating wheel packages on linux and mac. On those platforms
there are automated tools that move binary dependencies into a sandboxed
location for installing on a targeted system. We use that to get the USD shared
libraries into a safely redistributable package. Once the libraries are moved,
we need to update the pluginfo files to also point to the new locations.
'''
import argparse, base64, csv, fnmatch, hashlib, io, json, os, platform, zipfile


def is_mac_platform():
    return platform.system() == "Darwin"


def get_hash(data):
    # based on the rewrite_record function in auditwheel
    # https://github.com/pypa/auditwheel/blob/67ba9ff9c43fc6d871ee3e7e300125fe6bf282f3/auditwheel/wheeltools.py#L45
    # also, the encoding helpers found here
    # https://github.com/pypa/wheel/blob/master/src/wheel/util.py#L26
    digest = hashlib.sha256(data).digest()
    hashval = 'sha256=' + \
            base64.urlsafe_b64encode(digest).rstrip(b'=').decode('utf-8')
    size = len(data)
    return (hashval, size)


def process_plugin_dict(plugin_dict, libname, newname):
    # Check if this dict needs to be updated for the new library name.
    #
    # This is assuming the relative path structure that we currently get. I
    # think this should work for the way our wheels are currently built, and
    # the way USD is currently setting these paths internally. If either
    # changes this will break.
    if is_mac_platform():
        if f'libs/{libname}' in newname:
            # strip off a pxr/ at the front of the newname
            plugin_dict['LibraryPath'] = os.path.join('../../', newname[4:])
            return True
    else:
        if f'libs/{libname}-' in newname:
            # found a new location for this lib
            plugin_dict['LibraryPath'] = os.path.join('../../../', newname)
            return True
    return False


def update_pluginfo(contents, new_lib_names, new_pluginfo_hashes):
    # some USD json files begin with python style comments, and those aren't
    # legal json. For our purposes we'll strip them.
    while contents[0] == ord('#'):
        line_end = contents.find(ord('\n'))
        contents = contents[line_end+1:]

    json_doc = json.loads(contents.decode('utf-8'))
    changed = False
    for p in json_doc.get("Plugins", []):
        if "LibraryPath" in p:
            libpath = p["LibraryPath"]
            _, libname = os.path.split(libpath)
            libname, _ = os.path.splitext(libname)
            # libname is now like libsdf or libtf
            for newname in new_lib_names:
                changed = process_plugin_dict(p, libname, newname)
                if changed:
                    break

    if changed:
        return json.dumps(json_doc, indent=4).encode('utf-8')

    return contents


def update_record(contents, new_pluginfo_hashes):
    result = io.StringIO()
    csv_writer = csv.writer(result)
    csv_reader = csv.reader(io.StringIO(contents.decode('utf-8')))
    for row in csv_reader:
        if len(row) > 0 and row[0] in new_pluginfo_hashes:
            sha, size = new_pluginfo_hashes[row[0]]
            row[1] = sha
            row[2] = size
        csv_writer.writerow(row)
    return result.getvalue()


def parse_command_line():
    parser = argparse.ArgumentParser(
                        description=__doc__,
                        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("input_file", type=str, 
                        help="The input wheel archive to be updated")
    parser.add_argument("output_file", type=str, 
                        help="The output wheel archive to write to")
    args = parser.parse_args()
    return args.input_file, args.output_file


def main():

    input_file, output_file = parse_command_line()

    if not os.path.exists(input_file):
        raise RuntimeError("no such file " + input_file)

    # Loop over the file once to update pluginfos and compute new hash values
    temp_file_name = input_file + '_old_hashes'
    new_pluginfo_hashes = {}
    with zipfile.ZipFile(input_file, 'r') as input_wheel:
        with zipfile.ZipFile(temp_file_name, 'w', 
                             compression=input_wheel.compression) \
                                                        as output_wheel:
            # This seems blank, but can't hurt to keep it
            output_wheel.comment = input_wheel.comment

            if is_mac_platform():
                new_lib_names = fnmatch.filter(input_wheel.namelist(),
                                               '*/*.dylib')
            else:
                new_lib_names = fnmatch.filter(input_wheel.namelist(), 
                                               '*/lib*.so')

            for file_info in input_wheel.infolist():
                if fnmatch.fnmatch(file_info.filename, '*/plugInfo.json'):
                    print(f'processing: {file_info.filename}')
                    data = input_wheel.read(file_info)
                    contents = update_pluginfo(
                                    data, new_lib_names, new_pluginfo_hashes)

                    # update hash
                    new_pluginfo_hashes[file_info.filename] = get_hash(contents)

                    output_wheel.writestr(file_info, contents)
                else:
                    print(f'copying: {file_info.filename}')
                    # writestr also writes binary if passed a bytes instance
                    output_wheel.writestr(file_info, 
                                          input_wheel.read(file_info))

    # Loop over a second time this time keeping all files intact, but rewriting
    # the RECORD file containing the hashes
    with zipfile.ZipFile(temp_file_name, 'r') as input_wheel:
        with zipfile.ZipFile(output_file, 'w', 
                             compression=input_wheel.compression) \
                                                        as output_wheel:
            output_wheel.comment = input_wheel.comment

            for file_info in input_wheel.infolist():
                if fnmatch.fnmatch(file_info.filename, '*/RECORD'):
                    print(f'updating hashes in {file_info.filename}')
                    data = input_wheel.read(file_info)
                    contents = update_record(data, new_pluginfo_hashes)
                    output_wheel.writestr(file_info, contents)
                else:
                    output_wheel.writestr(file_info, 
                                          input_wheel.read(file_info))


if __name__ == '__main__':
    main()
