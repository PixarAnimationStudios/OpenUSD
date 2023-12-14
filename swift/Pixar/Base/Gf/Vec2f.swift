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

public typealias GfVec2f = Pixar.GfVec2f

public extension Pixar.Gf
{
  typealias Vec2f = GfVec2f
}

/**
 * # GfVec2f
 *
 * Basic type for a vector of 2 float components.
 *
 * Represents a vector of 2 components of type **float**.
 * It is intended to be fast and simple.
 */
extension GfVec2f: Scalar
{
  /// Axis count of the vector.
  public typealias AxisCount = Axis2

  /// Create a unit vector along the specified axis.
  public static func axis(_ axis: AxisCount) -> Self
  {
    switch axis
    {
      case .x:
        XAxis()

      case .y:
        YAxis()
    }
  }

  public mutating func set(_ s0: Float, _ s1: Float) -> Self
  {
    Set(s0, s1).pointee
  }

  public mutating func set(_ a: [ScalarType]) -> Self
  {
    Set(a).pointee
  }

  public func getArray() -> [ScalarType]
  {
    let buffer = UnsafeBufferPointer(start: GetArray(), count: GfVec2f.scalarCount)

    return Array(buffer)
  }

  public func getProjection(_ other: Self) -> Self
  {
    GetProjection(other)
  }

  public func getComplement(_ normal: Self) -> Self
  {
    GetComplement(normal)
  }

  public func getLengthSq() -> Float
  {
    GetLengthSq()
  }

  public func getLength() -> Float
  {
    GetLength()
  }

  public mutating func normalize(_ eps: Float) -> Float
  {
    Normalize(eps)
  }

  public func getNormalized(_ eps: Float) -> Self
  {
    GetNormalized(eps)
  }
}

extension GfVec2f: SIMD
{
  public typealias Scalar = Self.ScalarType
  public typealias SIMDStorage = SIMD2<Scalar>
  public typealias MaskStorage = SIMD2<Scalar>.MaskStorage

  public var scalarCount: Int { 2 }

  public var simd: SIMD2<Scalar>
  {
    get
    {
      SIMD2<Scalar>(
        Scalar(data()[0]), 
        Scalar(data()[1])
      )
    }
    set
    {
      dataMutating()[0] = Scalar(newValue[0])
      dataMutating()[1] = Scalar(newValue[1])
    }
  }

  public init(_ simd: SIMD2<Scalar>)
  {
    self.init()

    self.simd = simd
  }
}
