/* --------------------------------------------------------------
 * :: :  M  E  T  A  V  E  R  S  E  :                          ::
 * --------------------------------------------------------------
 * This program is free software; you can redistribute it, and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. Check out
 * the GNU General Public License for more details.
 *
 * You should have received a copy for this software license, the
 * GNU General Public License along with this program; or, if not
 * write to the Free Software Foundation, Inc., to the address of
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *       Copyright (C) 2023 Wabi Foundation. All Rights Reserved.
 * --------------------------------------------------------------
 *  . x x x . o o o . x x x . : : : .    o  x  o    . : : : .
 * -------------------------------------------------------------- */

@_exported import CxxStdlib

/* ---- Pixar.Base ---- */
@_exported import Arch
@_exported import Gf
// @_exported import Tf
@_exported import Js
@_exported import Plug
@_exported import Trace
@_exported import Vt
@_exported import Work

/* ----- Pixar.Usd ---- */
@_exported import Ar
@_exported import Kind
@_exported import Sdf
@_exported import Pcp
@_exported import Usd

/* ----- Pixar.NS ----- */
@_exported import pxr

/* -------------------- */

public extension Pixar
{
  /**
    # ``Arch``

    ## Overview

    **Arch** is a repository for all **architecture-dependent code**.
    It's purpose is to isolate all platform dependencies into one small library,
    serving as a common area for documentation of these multi-platform issues.
   */
  struct Arch
  {}

  /**
     # ``Js``

     ## Overview

     **Js** is the **JSON I/O** library, it contains methods for parsing and
     writing JSON data from C++, and converting between arbitrary recursive
     container structures.
   */
  struct Js
  {}

  /**
    # ``Tf``

    ## Overview

    **Tf** is the **tools foundations** library, it contains foundation
    classes and functions for all C/C++ software development.
   */
  struct Tf
  {}

  /**
    # ``Gf``

    ## Overview

    **Gf** is the **graphics foundations** library, it contains foundation
    classes and functions for working with the basic mathematical aspects
    of computer graphics.
   */
  struct Gf
  {}

  /**
    # ``Trace``

    ## Overview

    **Trace** is the **performance tracking** library, it contains utility classes
    for counting, timing, measuring, and recording events.
   */
  struct Trace
  {}

  /**
    # ``Work``

    ## Overview

    **Work** is intended to simplify the use of *multithreading* in the context of
    our software ecosystem.
   */
  struct Work
  {}

  /**
    # ``Plug``

    ## Overview

    **Plug** is the **plugin-in framework** implementation, the ``PlugPlugin`` class
    defines the interface to plug-in modules. The ``PlugRegistry`` class defines a
    mechanism for registering and looking up plug-in modules both automatically upon
    first use and manually at runtime by client calls to ``PlugRegistry.RegisterPlugins()``.
   */
  struct Plug
  {}

  /**
    # ``Vt``

    ## Overview

    **Vt** is the **value types** library, it provides classes that represent
    basic value types such as arrays, strings, and tokens. This library operates
    on the level of language data types and there are differences in the C++ and
    Python APIs.
   */
  struct Vt
  {}

  /**
    # ``Ar``

    ## Overview

    **Ar** is the **asset resolution** library, and is responsible for querying, reading, and
    writing asset data. It provides several interfaces that allow **USD** to access
    an asset without knowing how that asset is physically stored.
   */
  struct Ar
  {}

  /**
    # ``Kind``

    ## Overview

    The **Kind** library provides a runtime-extensible taxonomy known as **"Kinds"**.
    **Kinds** are just **TfToken** symbols, but the ``KindRegistry`` allows for organizing
    kinds into taxonomies of related/refined concepts, and the ``KindRegistry.GetBaseKind()``
    and ``KindRegistry.IsA()`` queries enable reasoning about the hierarchy and classifying
    objects by kind.
   */
  struct Kind
  {}

  /**
    # ``Sdf``

    ## Overview

    **Sdf** provides the foundations for serializing scene description to a reference text format,
    or a multitude of plugin-defined formats.  It also provides the primitive abstractions for
    interacting with scene description, such as ``SdfPath``, ``SdfLayer``, ``SdfPrimSpec``.
   */
  struct Sdf
  {}

  struct Ndr
  {}

  struct Sdr
  {}

  struct Pcp
  {}

  struct Usd
  {}

  struct UsdGeom
  {}

  struct UsdVol
  {}

  struct UsdMedia
  {}

  struct UsdShade
  {}

  struct UsdLux
  {}

  struct UsdRender
  {}

  struct UsdHydra
  {}

  struct UsdRi
  {}

  struct UsdSkel
  {}

  struct UsdUI
  {}

  struct UsdUtils
  {}

  struct UsdPhysics
  {}

  struct UsdAbc
  {}

  struct UsdDraco
  {}

  struct Garch
  {}

  struct CameraUtil
  {}

  struct PxOsd
  {}

  struct Glf
  {}

  struct UsdImagingGL
  {}

  struct UsdAppUtils
  {}

  struct Usdview
  {}
}

/* --- xxx --- */
