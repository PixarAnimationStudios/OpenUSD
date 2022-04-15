import re

import docutils.parsers.rst
from docutils import nodes
from sphinx.util.docutils import SphinxDirective

import os

repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "../.."))


class Parser(object):
    tokens = ("usddoc", "usdbytelayout")
    cache = {}
    inline_layout_pattern = re.compile(r"<(\w+) (\w+)>")
    byte_layout_pattern = re.compile(r"([\w0-9-]+) ([0-9]+|\?)")

    def __init__(self, element, identifier, source_file):
        super(Parser, self).__init__()
        self.element = element
        self.identifier = identifier
        self.source_file = source_file
        self.contents = []

    @classmethod
    def parse_cpp(cls, filepath, force=False):
        filepath = os.path.abspath(os.path.join(repo_root, filepath))
        base, ext = os.path.splitext(filepath)
        if ext == ".h":
            header = filepath
            filepath = base + ".cpp"
        else:
            header = base + ".h"

        if filepath in cls.cache and not force:
            return cls.cache[filepath]

        for path in (filepath, header):
            if not os.path.exists(path):
                raise IOError("Could not find {}".format(path))

            with open(path, "r") as fh:
                current = None
                for line in fh.readlines():
                    line = line.strip()
                    if not current and line.startswith("/*"):
                        line = line[2:].strip().split()
                        if len(line) < 2:
                            continue

                        if line[0].lower() not in cls.tokens:
                            continue

                        current = cls(line[0], line[1], path)
                        continue

                    if current and line.startswith("* "):
                        line = line[2:]

                    if current and line == "*":
                        line = "\n"

                    if current and line.startswith("*/"):
                        line = line[:-2].strip()
                        if line:
                            current.contents.append(line)
                        cls.cache.setdefault(filepath, {}).setdefault(current.element, {})[current.identifier] = current
                        current = None
                        continue

                    if not current:
                        continue

                    current.contents.append(line)

        results = cls.cache.get(filepath)
        if not results:
            raise IOError("Could not find any compatible docs in {}".format(path))

        return results

    @classmethod
    def fetch_cpp_doc(cls, src, docid):
        return cls.get_cached(src, "usddoc", docid).parse()

    @classmethod
    def fetch_cpp_byte_layout(cls, src, docid):
        return cls.get_cached(src, "usdbytelayout", docid).parse()

    @classmethod
    def get_cached(cls, src_file, doc_type, doc_id):
        found = cls.parse_cpp(src_file)
        if not found:
            raise RuntimeError(f"Could not parse {src_file}")

        parsed_doc_objects = found.get(doc_type)
        if not parsed_doc_objects:
            raise RuntimeError(f"Could not find any {doc_type} elements in {src_file}")

        doc_object = parsed_doc_objects.get(doc_id)
        if not doc_object:
            raise  RuntimeError(f"Could not find {doc_type} with id {doc_id} in {src_file}")

        return doc_object


    def parse(self):
        if self.element == "usddoc":
            return self._parse_usddoc()
        elif self.element == "usdbytelayout":
            return self._parse_usdbytelayout()
        else:
            raise RuntimeError("Unknown element type: {}".format(self.element))

    def format_paragraph(self, contents):
        parser = docutils.parsers.rst.Parser()
        settings = docutils.frontend.OptionParser(
            components=(docutils.parsers.rst.Parser,)
        ).get_default_values()
        document = docutils.utils.new_document(self.identifier, settings)
        parser.parse(contents, document)

        nodes = document.children

        return nodes

    def _parse_usddoc(self):
        _nodes = []
        contents = ""
        for line in self.contents:
            match = self.inline_layout_pattern.match(line)
            if match:
                groups = match.groups()
                if groups[0] in self.tokens:
                    if contents:
                        _nodes.extend(self.format_paragraph(contents))
                        contents = ""

                    _nodes.extend(Parser.get_cached(self.source_file, groups[0], groups[1]).parse())
                    continue


            contents += line + "\n"

        if contents:
            _nodes.extend(self.format_paragraph(contents))






        return _nodes

    def _parse_usdbytelayout(self):
        rows = []
        current = []
        current_size = 0
        for line in self.contents:
            match = self.byte_layout_pattern.match(line.strip())
            if not match:
                continue

            key, size = match.groups()

            if size == "?":
                if current:
                    rows.append(current)
                    current_size = 0
                    current = []

                rows.append([(key, 16)])
                continue

            size = int(size)

            # if we still have space in this row
            if current_size < 16 and current_size + size > 16:
                use = 16 - current_size
                current.append((key, use))
                size -= use
                current_size += use

            # if we're at 16
            if current_size >= 16:
                current_size = 0
                rows.append(current)
                current = []

            current.append((key, size))
            current_size += size

        if current_size:
            rows.append(current)


        content = """<table class="byte-table">
                <thead>
                    <tr>
                        <th></th>
                        <th>0x0</th>
                        <th>0x1</th>
                        <th>0x2</th>
                        <th>0x3</th>
                        <th>0x4</th>
                        <th>0x5</th>
                        <th>0x6</th>
                        <th>0x7</th>
                        <th>0x8</th>
                        <th>0x9</th>
                        <th>0xa</th>
                        <th>0xb</th>
                        <th>0xc</th>
                        <th>0xd</th>
                        <th>0xe</th>
                        <th>0xf</th>
                    </tr>
                </thead>
                <tbody>"""

        for i, row in enumerate(rows):
            label = "{0:#0{1}x}".format(i * 16, 6)
            content += f"\n<tr>\n\t<th>{label}</th>"
            total = 0

            for item, length in row:
                total += length
                content += f"\n\t<td colspan=\"{length}\">{item}</td>"

            if total < 16:
                try:
                    item = rows[i+1][0][0]
                except IndexError:
                    item = ""
                content += f"\n\t<td colspan=\"{16-total}\">{item}</td>"
            content += "\n</tr>"


        _nodes = []
        content += """</tbody>
            </table>"""

        _nodes.append(nodes.raw('', content, format='html'))
        return _nodes

    def __repr__(self):
        return "{}::({},{} in {})".format(super(Parser, self).__repr__(), self.element, self.identifier, self.source_file)


class UsdCppDoc(SphinxDirective):
    has_content = True

    def run(self):
        src, docid = self.content[0].split()
        return Parser.fetch_cpp_doc(src, docid)


class UsdCppByteLayout(SphinxDirective):
    has_content = True

    def run(self):
        src, docid = self.content[0].split()
        return Parser.fetch_cpp_byte_layout(src, docid)

class UsdCrateTypes(SphinxDirective):
    has_content = True

    def run(self):
        crate_data_types_file = os.path.join(repo_root, "pxr/usd/usd/crateDataTypes.h")
        types_file = os.path.join(repo_root, "pxr/usd/sdf/types.h")

        with open(types_file) as fp:
            scalar_types = set()
            dimensioned_types = {}
            add_to_scalar = False
            add_to_dimensional = False
            for line in fp.readlines():
                if "_SDF_SCALAR_VALUE_TYPES" in line:
                    if add_to_scalar or add_to_dimensional:
                        break

                    add_to_scalar = True
                    add_to_dimensional = False
                elif "_SDF_DIMENSIONED_VALUE_TYPES" in line:
                    add_to_scalar = False
                    add_to_dimensional = True

                if not (add_to_scalar or add_to_dimensional):
                    continue

                line = line.strip()
                if not line.startswith("(("):
                    continue

                token, _, cpp_type, dimensions =  line.split(",  ")
                token = token[2:]

                if add_to_scalar:
                    scalar_types.add(token)
                    continue

                dimensions = dimensions.strip().split()[0]
                dimensioned_types[cpp_type.strip()] = dimensions[1:-1]



        table = nodes.table(cols=5)
        group = nodes.tgroup()
        head = nodes.thead()
        body = nodes.tbody()

        table += group
        group += nodes.colspec(colwidth=4)
        group += nodes.colspec(colwidth=2)
        group += nodes.colspec(colwidth=4)
        group += nodes.colspec(colwidth=3)
        group += nodes.colspec(colwidth=3)
        group += head
        group += body

        row = nodes.row()
        row += nodes.entry('', nodes.paragraph('', nodes.Text('Name')))
        row += nodes.entry('', nodes.paragraph('', nodes.Text('ID')))
        row += nodes.entry('', nodes.paragraph('', nodes.Text('C++ Type')))
        row += nodes.entry('', nodes.paragraph('', nodes.Text('Supports Array')))
        row += nodes.entry('', nodes.paragraph('', nodes.Text('Dimensions')))
        head += row


        data = [("Invalid", 0, "", False, None)]

        with open(crate_data_types_file, "r") as fp:
            for line in fp.readlines():
                if not line.startswith("xx("):
                    continue

                line = line.strip()[3:-1]
                name, enum_index, cpp_type, array_type = line.split(",")

                enum_index = int(enum_index.strip())
                array_type = array_type.strip() == "true"
                cpp_type = cpp_type.strip()

                data.append([name.strip(), enum_index, cpp_type, array_type, dimensioned_types.get(cpp_type)])

        data.append(["NumTypes", enum_index+1, '', False, None])


        for item in data:
            row = nodes.row()
            row += nodes.entry('', nodes.paragraph('', nodes.Text(item[0])))
            row += nodes.entry('', nodes.paragraph('', nodes.Text(str(item[1]))))
            row += nodes.entry('', nodes.paragraph('', nodes.Text(item[2])))
            row += nodes.entry('', nodes.paragraph('', nodes.Text("✓" if item[3] else "")))
            row += nodes.entry('', nodes.paragraph('', nodes.Text(item[4] or "")))
            body += row

        return [table]

def setup(app):
    app.add_directive('usdcppdoc', UsdCppDoc)
    app.add_directive('usdcppbytelayout', UsdCppByteLayout)
    app.add_directive('usdcratetypes', UsdCrateTypes)

    return {
        "version": "0.1",
        "parallel_read_safe": True,
        "parallel_write_safe": True,
    }
