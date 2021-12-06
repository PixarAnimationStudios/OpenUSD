# Style and Build Guide for USD Documentation

USD uses [Sphinx](https://www.sphinx-doc.org/en/master/) to generate its 
user documentation and [Doxygen](https://www.doxygen.nl/manual/index.html)
for its API documentation.

Following are introductions to Sphinx, how we use it and style our documentation,
and how to build it locally.


## Sphinx

The user documentation is specified in [restructured text (rst)](https://www.sphinx-doc.org/en/master/usage/restructuredtext/basics.html),
and makes use of the following Sphinx extensions:

* [ReadTheDocs theme](https://sphinx-rtd-theme.readthedocs.io/en/stable/)
* [Sphinx Panels](https://sphinx-panels.readthedocs.io/en/latest/)
* [Doxylink](https://github.com/sphinx-contrib/doxylink)

You will need to ensure your installation of sphinx includes these three packages
for a documentation build to complete properly.

We have found the following references helpful:

* [Home page for Sphinx docs](https://www.sphinx-doc.org/en/master/)
* [Documentation on built-in roles in Sphinx](https://www.sphinx-doc.org/en/master/usage/restructuredtext/roles.html)
* [Documentation on built-in roles and directives in rst](https://docutils.sourceforge.io/docs/ref/rst/directives.html)


## USD Sphinx Style Guide

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
* Use ["doc:"](https://docs.readthedocs.io/en/stable/guides/cross-referencing-with-sphinx.html#the-doc-role) to link to other documents. 
  For example, to link to the "Hello World" tutorial in `tut_helloworld.rst`, 
  use:
  ```rst
  :doc:`tut_helloworld` OR
  :doc:`Custom Link Text <tut_helloworld>`
  ```
* Use the ":usdcpp:" role to link to Doxygen API docs for a specified function 
  or class. This is a custom role created using the doxylink extension for
  sphinx. For example, to link to the UsdStage API docs, use:
  ```rst
  :usdcpp:`UsdStage` OR
  :usdcpp:`Custom Link Text <UsdStage>`
  ```
* Please conform the title and section headers to the following [convention 
  required by the ReadTheDocs template](https://documentation-style-guide-sphinx.readthedocs.io/en/latest/style-guide.html#headings) to work well with the 
  navigation bar:
  
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
  .. code-block:: none
  ```
* Formatting Quickstart
  * `` *italics* `` _italics_
  * `` **bold** `` __bold__
  * `` :bi:`bolditalics` `` ___bolditalics___
  * `` :mono:`monospace` `` `monospace`
  * The `rolesAndUtils.rst` file defines a number of "roles" (e.g. like 
    `:mono:`) used in these pages for various kinds of objects, including:
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

`build_usd.py` can be used to build the USD doxygen documentation.  We may 
eventually enable it to build the Sphinx documentation as well, but we currently
do not want to introduce the Sphinx (and packages) dependency.  Following
are instructions for building the complete documentation manually.

### Sphinx Setup

If your system already has sphinx installed, you must ensure the installation 
has the `sphinx-rtd-theme`, `sphinx_panels`, and `sphinxcontrib-doxylink`
extension packages installed.  You can instead install sphinx locally 
(instructions for linux and python3) like so:
```
pip3 install --user sphinx sphinx-rtd-theme sphinx_panels sphinxcontrib-doxylink
    
# Add to .cshrc or equivalent command in your shell's configuration file
# Make sure to run rehash or open a new shell to pick up this change.
setenv PATH $HOME/.local/bin:$PATH
    
# Python also needs to find your local install
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
 