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
 * # System Functions
 *
 * Functions that encapsulate differing low-level system calls. */
public extension Pixar.Arch
{
  /// Return current working directory as a string.
  static func getCwd() -> String
  {
    String(Pixar.ArchGetCwd())
  }

  /// Return the path to the program's executable.
  static func getExecutablePath() -> String
  {
    String(Pixar.ArchGetExecutablePath())
  }

  /// Return the system's memory page size. Safe to assume power-of-two.
  static func getPageSize() -> Int
  {
    Int(Pixar.ArchGetPageSize())
  }
}
