# Generating Character Classes from the Unicode Database

To properly process UTF-8 encoded strings, the system needs to know what code
points fall into what Unicode character class.  This is useful for e.g.,
processing identifiers to determine whether the first character is represented
by a Unicode code point that falls into the `XidStart` Unicode character class.
Unicode defines a maximum of 17 * 2^16 code points, and we need an efficient
way of representing character class containment for each of these code points.
The chosen data structures of interest are implemented in the
`pxr/base/tf/unicode/unicodeCharacterClasses.template.cpp` file, but the code
points of interest for each character class must be generated from a source
version of the Unicode database.

This directory contains a script `tfGenCharacterClasses.py` that will read in
character class information from a source version of the Unicode database and
generate the `pxr/base/tf/unicodeCharacterClasses.cpp` file from the provided
`pxr/base/tf/unicode/unicodeCharacterClasses.template.cpp` file.  The Unicode
database provides a post-processed file called `DerivedCoreProperties.txt` in
its core collateral.  For the script to function, this file must be present
locally on disk (see below for information about where to obtain the source
Unicode character class data).  Once the script is present locally, the
character classes can be generated via the following command:

```
# example run from the pxr/base/tf/unicode directory
python tfGenCharacterClasses.py --srcDir <path/to/DerivedCoreProperties.txt>
    --destDir .. --srcTemplate unicodeCharacterClasses.template.cpp
```

This command will overwrite the current 
`pxr/base/tf/unicodeCharacterClasses.cpp` file with the newly generated
version.

**NOTE: This script need only be run once when upgrading to a new**
**Unicode version**

## Source Unicode Database

The Unicode Character Database consists of a set of files representing
Unicode character properties and can be found at https://unicode.org/ucd/
and the `DerivedCoreProperties.txt` file can be obtained in the `ucd`
directory of the collateral at whatever version you are interested in
supporting.

The current version of `pxr/base/tf/unicodeCharacterClasses.cpp`
was generated from `DerivedCoreProperties.txt` for Unicode Version 15.1.0.