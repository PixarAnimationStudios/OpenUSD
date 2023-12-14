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

public typealias TfToken = Pixar.TfToken

extension Pixar.TfToken 
{
  private borrowing func GetStringCopy() -> std.string 
  {
    return __GetStringUnsafe().pointee
  }

  public var string: std.string 
  {
    GetStringCopy()
  }
}

public extension Pixar.Tf
{
  /**
   * # Pixar.Tf.Token
   *
   * ### Overview
   *
   * **Token** for efficient comparison, assignment, and hashing of known strings.
   *
   * A TfToken is a handle for a registered string, and can be compared,
   * assigned, and hashed in constant time. It is useful when a bounded
   * number of strings are used as fixed symbols (but never modified). */
  typealias Token = TfToken
}
