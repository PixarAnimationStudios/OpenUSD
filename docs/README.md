# Style and Build Guide for USD Documentation

USD uses [Sphinx](https://www.sphinx-doc.org/en/master/) to generate  
user documentation and [Doxygen](https://www.doxygen.nl/manual/index.html)
for API documentation. This README describes how to configure Sphinx, how
to write user documentation in reStructuredText that follows USD's recommended
documentation style guide, and how to build Sphinx documentation locally.

## Sphinx

USD's user documentation is authored using [reStructuredText (rst)](https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html), although [MyST markdown](https://myst-parser.readthedocs.io/en/v0.17.1/syntax/syntax.html) 
may also be used. Sphinx is used to build the reStructuredText into HTML, which
is published on [openusd.org](https://openusd.org/release/index.html). 

USD's Sphinx build uses the following Sphinx extensions:

* [ReadTheDocs theme](https://sphinx-rtd-theme.readthedocs.io/en/stable/)
* [Sphinx Panels](https://sphinx-panels.readthedocs.io/en/latest/)
* [Doxylink](https://github.com/sphinx-contrib/doxylink)
* [MyST](https://myst-parser.readthedocs.io/en/v0.17.1/index.html)

See "Sphinx Setup" below for how to install Sphinx and the necessary extensions.

For working with Sphinx, we have found the following references helpful:

* [Home page for Sphinx docs](https://www.sphinx-doc.org/en/master/)
* [Documentation on built-in roles in Sphinx](https://www.sphinx-doc.org/en/master/usage/restructuredtext/roles.html)
* [Documentation on built-in roles and directives in rst](https://docutils.sourceforge.io/docs/ref/rst/directives.html)

## USD Sphinx Style Guide

We recommend using the following style guidelines when editing or authoring
rst content. The rst directives mentioned in this section can also be used in
Myst markdown files using a [different syntax](https://myst-parser.readthedocs.io/en/v0.17.1/syntax/syntax.html#syntax-directives).

* Use [":ref:"](https://docs.readthedocs.io/en/stable/guides/cross-referencing-with-sphinx.html#the-ref-role) to link to section headers in other documents. 
  For example, to link to the "API Schema" entry in `glossary.rst`, use:
  ```rst
  :ref:`glossary:API Schema` OR
  :ref:`Custom Link Text <glossary:API Schema>`
  ```
  For __intra-page links__, however, you must (and only in this case) use the
  "trailing underbar" syntax:
  ```rst
  `Custom Link Text <glossary:API Schema>`_
  ```
* Use [":doc:"](https://docs.readthedocs.io/en/stable/guides/cross-referencing-with-sphinx.html#the-doc-role) to link to other documents. 
  For example, to link to the "Hello World" tutorial in `tut_helloworld.rst`, 
  use:
  ```rst
  :doc:`tut_helloworld` OR
  :doc:`Custom Link Text <tut_helloworld>`
  ```
* Use the ":usdcpp:" role to link to Doxygen API docs for a specified function 
  or class. This is a custom role created using the doxylink extension for
  Sphinx. For example, to link to the UsdStage API docs, use:
  ```rst
  :usdcpp:`UsdStage` OR
  :usdcpp:`Custom Link Text <UsdStage>`
  ```
* Please conform the title and section headers to the following [convention 
  required by the ReadTheDocs template](https://documentation-style-guide-sphinx.readthedocs.io/en/latest/style-guide.html#headings) 
  to work well with the navigation bar:
  
  Use the following symbols to create headings H1 - H6:
  1. \# with overline
  2. \* with overline
  3. =
  4. -
  5. ^
  6. "
* The code-block directive takes a parameter to specify syntax highlighting. 
  Commonly-used values are: python , cpp , usd , usda, none . e.g.:
  ```rst
  .. code-block:: python

     from pxr import Usd, UsdGeom
     stage = Usd.Stage.CreateNew('HelloWorld.usda')
     xformPrim = UsdGeom.Xform.Define(stage, '/hello')
     spherePrim = UsdGeom.Sphere.Define(stage, '/hello/world')
     stage.GetRootLayer().Save()
  ```
* Formatting Quickstart
  * `` *italics* `` _italics_
  * `` **bold** `` __bold__
  * `` :bi:`bolditalics` `` ___bolditalics___
  * `` :mono:`monospace` `` `monospace`
  * The `rolesAndUtils.rst` file defines a number of "roles" (e.g. like 
    `:mono:`) used for various kinds of objects, including:
    * `:sdfpath:` for scene description paths, like `</World/anim/chars/Bob>`
    * `:filename:` for files/assets
    * `:python:` for __inline__ python code
    * `:cpp:` for inline C++ code that does __not__ indicate an API method/class
      that can be located in the USD doxygen documentation 
    * `:code:` for inline code that is ambiguous with respect to language and
      does not link to the USD doxygen documentation


## Content Notes

1. The `contributors.rst` file references a `contributors.html` file, which is 
   maintained by Pixar internally, and cannot receive pull requests.

## Building Documentation Locally

`build_usd.py` can be used to build the USD doxygen API documentation, but not 
the Sphinx documentation. We may eventually enable it to build the Sphinx 
documentation as well, but we currently do not want to introduce the Sphinx 
(and packages) dependency to the build. Use the following instructions for 
building the complete documentation manually.

### Sphinx Setup

Depending on your development platform, there are several ways to install Sphinx
as described in [Installing Sphinx](https://www.sphinx-doc.org/en/master/usage/installation.html).

Once you have Sphinx installed, ensure you have the  `sphinx-rtd-theme`, 
`sphinx_panels`, `sphinxcontrib-doxylink`, and `myst_parser`extension packages 
installed. Depending on your local Python environment, use either conda or pip 
to install the extensions. The following command uses pip to install __both__ 
Sphinx and the required extensions:
```
pip3 install --user sphinx sphinx-rtd-theme sphinx_panels sphinxcontrib-doxylink myst-parser
```

Make sure to add Sphinx to your path:

```    
# Add to .cshrc or your shell's equivalent configuration file
# Make sure to run rehash or open a new shell to pick up this change.
setenv PATH $HOME/.local/bin:$PATH
    
# On Linux, Python also needs to find your local install
setenv PYTHONPATH $HOME/.local:$PYTHONPATH
```

### Pulling in API documentation

To enable links to Doxygen API documentation in the user documentation,
set the following environment variables before running sphinx-build:

- __USD_DOXYGEN_TAG_FILE__

  This should be set to the path of the .tag file generated by the Doxygen
  documentation build. Sphinx will read this file to determine the HTML
  files containing documentation for a given class or function.

  By default, the USD build installs this file to USD_INSTALL_ROOT/docs/USD.tag .

- __USD_DOXYGEN_PREFIX__

  This should be set to __a URI for__ the directory containing the Doxygen HTML
  files.  Sphinx will prepend all links to Doxygen HTML files with this prefix.

  By default, the USD build installs these files to USD_INSTALL_ROOT/docs/doxy_html

For example, for a USD build installed in USD_INSTALL_ROOT that is not being
served by a web-server:
```
setenv USD_DOXYGEN_TAG_FILE USD_INSTALL_ROOT/docs/USD.tag
setenv USD_DOXYGEN_PREFIX file://USD_INSTALL_ROOT/docs/doxy_html
```
If these env vars are not set, the user documentation will still build but
Sphinx will issue some warnings, and links to API documentation in the
user documentation will not appear.


### Building User Documentation with Sphinx

To build the user documentation to `/tmp/usddocs`, you must be in the directory
containing this README and the rst source files, and then:
```
sphinx-build . /tmp/usddocs
```

Sphinx tracks changed files, so subsequent builds will be faster than the first.

### Generating Schema User Docs

Use the python/genSchemaDocs.py tool to generate Sphinx content files from 
USD schemas. The process for authoring schema user doc is as follows:

- Create (if it doesn't already exist) a sub-directory named "userDoc" where
  your schema.usda lives.
- In your userDoc directory, create (or update) overview "guide" markdown files 
  for your schemas, using the 
  [Myst Markdown](https://myst-parser.readthedocs.io/en/v0.15.1/sphinx/intro.html) 
  format. Add any local image files to the user_doc directory as well.
- In your userDoc directory, create (or update) "schemaUserDoc.usda". This
  will contain reference user doc for your schema classes and properties. Your
  schemaUserDoc.usda should sublayer your schema.usda, and provide overs for
  schema classes you want to add user doc for. Each over should add "userDoc" 
  and "userDocBrief" customData metadata strings for each schema class and 
  property. The "userDoc" string is used to generate the Sphinx output, and
  should be in 
  [Myst Markdown](https://myst-parser.readthedocs.io/en/v0.15.1/sphinx/intro.html) 
  format. The "userDocBrief" string is not used for Sphinx output, and is used
  for brief user doc presented in the UI of DCC Tools. The custom data will
  look something like the following:

```
over SchemaClass "SchemaClass" (
    customData = {
        string userDocBrief = """A short sentence or two describing the schema."""
        string userDoc = """A longer set of paragraphs describing the schema
in detail. Use Markdown format to include images, links, etc. Use Myst Markdown
to add Sphinx directives like references and labels.
"""
)
{
    asset schemaAssetProperty (
      customData = {
          string userDocBrief = """A short sentence or two describing the 
  property."""
          string userDoc = """A longer set of paragraphs describing the property
  in detail, again using Markdown and Sphinx directives as needed."""
      }
    )
}
```  

  Your "schemaUserDoc.usda" should also provide a "GLOBAL" prim that contains
  custom metadata for the schema domain name ("libraryName") and a short title 
  for the table-of-contents page for the generated Sphinx files 
  ("userDocTitle"). For example, for usdVol, the GLOBAL over in 
  schemaUserDoc.usda looks like the following:

```
over "GLOBAL" (
    customData = {
        string libraryName = "usdVol"
        string userDocTitle = "Volumes"
    }
)
{
}
```  

See pxr/usd/usdVol/userDoc/ for examples of the files mentioned above. Some
additional notes:

- Don't have any leading whitespace in your multiline doc strings. The 
generation process doesn't strip this out (because that would break spacing in 
things like codeblocks), and the extra whitespace will show up in the generated 
HTML.
- Don't add your own markdown headers (e.g. "# Title") in schemaUserDoc.usda. 
The generation tool automatically creates headers and Sphinx labels for 
classes and properties, and extra headers might result in confusing output.
You don't need to add RST labels for schema classes and attributes 
- The generation tool automatically adds Sphinx labels for classes and 
properties, using a `<schema name>_<property name>` format.
- In your overview guide markdown files, feel free to add labels to any 
sections you might need to link to from your schemaUserDoc.usda content. 
Using Myst Markdown, the syntax is "(label name)=" on its own line.

To generate the Sphinx files from your schema user doc files, run the 
genSchemaDocs.py tool pointing it at your userDocs directory and the 
desired output path. For example, if your current working directory was
docs/, you could generate usdVol Sphinx files into 
docs/user_guides/schemas/usdVol/ using the following command:

```
python python/genSchemaDocs.py ../pxr/usd/usdVol/userDocs user_guides/schemas -t
```

This will parse the content in pxr/usd/usdVol/userDocs and generate Sphinx
files into user_guides/schemas/usdVol, along with a usdVol_toc.rst 
table-of-contents file that can be included in any Sphinx RST file (like
docs/user_guides/schemas/index.rst).

Current content in docs/user_guides/schemas have been generating using the 
genSchemaDocs.py tool. 