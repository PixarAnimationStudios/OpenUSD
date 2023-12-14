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

public protocol Axis: CaseIterable, Dimensional
{
  func getAxis() -> Int

  static var dimension: Int { get }
}

public enum Axis2: Int, Axis
{
  case x = 0
  case y = 1

  public typealias AxisCount = Axis2

  public static var dimension: Int { 2 }

  public func getAxis() -> Int
  {
    switch self
    {
      case .x: 0
      case .y: 1
    }
  }
}

public enum Axis3: Int
{
  case x = 0
  case y = 1
  case z = 2

  public typealias AxisCount = Axis3

  public static var dimension: Int { 3 }

  func getAxis() -> Int
  {
    switch self
    {
      case .x: 0
      case .y: 1
      case .z: 2
    }
  }
}

public enum Axis4: Int
{
  case x = 0
  case y = 1
  case z = 2
  case w = 3

  public typealias AxisCount = Axis4

  public static var dimension: Int { 4 }

  func getAxis() -> Int
  {
    switch self
    {
      case .x: 0
      case .y: 1
      case .z: 2
      case .w: 3
    }
  }
}
