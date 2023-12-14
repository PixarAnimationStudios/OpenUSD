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

/**
 * # Memory Management
 *
 * Functions having to do with memory allocation / handling. */
public extension Pixar.Arch
{
  /// Return suitably aligned memory size.
  ///
  /// Requests to ``malloc()`` or ``::new`` for a given size are often rounded
  /// upward.
  ///
  /// - Parameter byteCount: The number of bytes to request.
  ///
  /// - Returns: The amount that would actually be consumed by the system to satisfy it.
  /// This is needed for efficient user-defined memory management.
  ///
  static func alignMemory(of byteCount: Int) -> Int
  {
    Pixar.ArchAlignMemorySize(byteCount)
  }

  /// Align memory to the next "best" alignment value.
  ///
  /// This will take a pointer and bump it to the next ideal alignment
  /// boundary that will work for all data types.
  ///
  /// - Parameter base: The base pointer needing alignment.
  ///
  /// - Returns: A pointer to the aligned memory.
  ///
  static func alignMemory(of base: UnsafeMutableRawPointer!) -> UnsafeMutableRawPointer!
  {
    Pixar.ArchAlignMemory(base)
  }

  /// Aligned memory allocation.
  ///
  /// - Parameter byteCount: The number of bytes to allocate.
  /// - Parameter alignment: The alignment of the new region of allocated memory.
  ///
  /// - Returns: A pointer to the newly allocated memory.
  ///
  static func alignedAlloc(byteCount: Int, alignment: Int) -> UnsafeMutableRawPointer!
  {
    Pixar.ArchAlignedAlloc(alignment, byteCount)
  }

  /// Free memory allocated by ArchAlignedAlloc.
  ///
  /// This will take a pointer returned by ArchAlignedAlloc and free it.
  ///
  /// - Parameter pointer: The pointer to free.
  ///
  static func alignedFree(pointer: inout UnsafeMutableRawPointer!)
  {
    Pixar.ArchAlignedFree(pointer)
    pointer = nil
  }
}
