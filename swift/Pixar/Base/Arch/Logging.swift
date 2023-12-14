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

/* --- xxx --- */

public final class PXRMsg
{
  private init()
  {}

  public static let Log = PXRMsg()
}

/* --- xxx --- */

public extension PXRMsg
{
  func point(_ subject: String, to msgArgs: Any...)
  {
    print(Colors.cyan.rawValue + subject + Colors.default.rawValue +
      Colors.yellow.rawValue + String(repeating: " ", count: max(35 - subject.count, 1)) + "-> " + Colors.default.rawValue +
      Colors.magenta.rawValue + "\(msgArgs)" + (Colors.default.rawValue + ""))
  }

  func info(_ msgArgs: Any...)
  {
    log(parseArgs(args: msgArgs), type: "INFO", color: .blue)
  }

  func debug(_ msgArgs: Any...)
  {
    log(parseArgs(args: msgArgs), type: "DEBUG", color: .magenta)
  }

  func success(_ msgArgs: Any...)
  {
    log(parseArgs(args: msgArgs), type: "SUCCESS", color: .green)
  }

  func warn(_ msgArgs: Any...)
  {
    log(parseArgs(args: msgArgs), type: "WARNING", color: .yellow)
  }

  func error(_ msgArgs: Any...)
  {
    log(parseArgs(args: msgArgs), type: "ERROR", color: .red)
  }
}

/* --- xxx --- */

public extension PXRMsg
{
  private func log(_ message: String, type: String, color: Colors = .default)
  {
    print(color.rawValue + "[ " + type + " ]" + Colors.default.rawValue + " " + color.rawValue + message + (Colors.default.rawValue + ""))
  }

  private func parseArgs(args: [Any]) -> String
  {
    var argIndex = 0
    var message = ""
    for arg in args
    {
      if argIndex >= 1
      {
        message += " "
        message += "\(arg)"
      }
      else
      {
        message += "\(arg)"
      }

      argIndex += 1
    }

    return message
  }
}

/* --- xxx --- */

public extension PXRMsg
{
  enum Colors: String
  {
    case black = "\u{001B}[0;30m"
    case red = "\u{001B}[0;31m"
    case green = "\u{001B}[0;32m"
    case yellow = "\u{001B}[0;33m"
    case blue = "\u{001B}[0;34m"
    case magenta = "\u{001B}[0;35m"
    case cyan = "\u{001B}[0;36m"
    case white = "\u{001B}[0;37m"
    case `default` = "\u{001B}[0;0m"

    func name() -> String
    {
      switch self
      {
        case .black: "Black"
        case .red: "Red"
        case .green: "Green"
        case .yellow: "Yellow"
        case .blue: "Blue"
        case .magenta: "Magenta"
        case .cyan: "Cyan"
        case .white: "White"
        case .default: "Default"
      }
    }

    static func all() -> [Colors]
    {
      [.black, .red, .green, .yellow, .blue, .magenta, .cyan, .white]
    }
  }

  func testColors()
  {
    for c in Colors.all()
    {
      PXRMsg.Log.log("This is printed in " + c.name(), type: "TEST", color: c)
    }
  }
}

/* --- xxx --- */
