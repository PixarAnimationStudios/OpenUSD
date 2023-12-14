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

public protocol Scalar: Dimensional
{
  /// Scalar element type.
  associatedtype ScalarType: Numeric

  /// Default constructor does no initialization.
  init()

  /// Initialize all elements to a single value.
  init(_ value: ScalarType)

  /// Indexing.
  subscript(_: Int) -> ScalarType { get set }

  /// Create a unit vector along the specified axis.
  static func axis<T>(_ axis: T) -> Self where T == AxisCount

  /// Set all elements with explicit arguments.
  mutating func set(_ s0: ScalarType, _ s1: ScalarType) -> Self

  // Set all elements with an array.
  mutating func set(_ a: [ScalarType]) -> Self

  /// Direct data access.
  func getArray() -> [ScalarType]

  /// Returns the projection of this onto other.
  /// That is, the component of this that is parallel to other.
  func getProjection(_ other: Self) -> Self

  /// Returns the component of this that is orthogonal to other.
  /// That is, the component of this that is perpendicular to other.
  func getComplement(_ normal: Self) -> Self

  /// Squared length.
  func getLengthSq() -> ScalarType

  /// Returns the length of this vector.
  func getLength() -> ScalarType

  /// Normalizes the vector in place to unit length, returning the
  /// length before normalization. If the length of the vector is
  /// smaller than the supplied eps, the vector is set to the null
  /// vector and zero is returned. The vector is unchanged otherwise.
  mutating func normalize(_ eps: ScalarType) -> ScalarType

  /// Returns a normalized (unit-length) copy of this vector.
  func getNormalized(_ eps: ScalarType) -> Self
}
