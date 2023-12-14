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

public extension Pixar.Ar
{
  static func getAllResolvers() -> [String]
  {
    // 1. we get the base type.
    let base = Pixar.TfType.FindByName("ArResolver")

    guard
      // 2. we verify the base type is valid.
      let all = base.pointee.IsUnknown() == false
        // 3. we get all that derive from the base type.
        ? Pixar.TfType.GetDirectlyDerivedTypes(base.pointee) : nil,
      // 4. we ensure the list is not empty.
      all().empty() == false
    else { return [] }

    return all().map
    {
      // return list of all type names.
      String($0.GetTypeName().pointee)
    }
  }
}
