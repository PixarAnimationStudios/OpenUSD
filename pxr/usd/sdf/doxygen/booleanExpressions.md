\page Sdf_Page_BooleanExpressions Boolean Expressions

### Boolean Expression Language
The syntax for boolean expression is intended to be concise and recognizable to those familiar with C/C++. An expression can be a constant value, a variable, or operations applied to one or more sub-expressions. The final result of evaluating an expression is always a boolean value.

#### Constants
The simplest allowed expressions are constant values. A constant can be a boolean value, a numeric value, or a string.
- `true`,`false`
- `12.34`, `42`
- `"quoted string"`

##### Strings
Strings may be either single- or double-quoted. Non-printable characters such as newlines must be escaped. Only single-line strings are permitted (no unescaped newline characters).
- `"double-quoted string"`
- ``'single-quoted string'``
- `"string with\nescaped newline"`

#### Variables
Variables are what allow expressions to be dynamic, evaluating to different values over time. A variable is expressed as a [USD property name](https://openusd.org/release/glossary.html#usdglossary-property); the permitted syntax for variables is identical to that of properties as supported by [`SdfPath`](https://openusd.org/dev/api/class_sdf_path.html#details).
- `simpleVariable`
- `namespaced:variables`

#### Binary Operators
A binary operator is applied to two subexpressions and is written with infix notiation: `leftExpression op rightExpression`. The supported operators are `>`, `>=`, `<`, `<=`, `==`, `!=`, `&&`, `||`.
- `numOps != 3`
- ``mode == 'default'``
- `width > 10.0`
- ``numOps != 3 && mode == 'default'``
- `width > 10.0 || height > 10.0`

#### Unary Operators
A unary operator applies to a single subexpression. The boolean complement/invert (`!`) is currently the only supported unary operator.
- `!foo`
- `foo == !bar`

#### Parenthesized expressions
Parenthesis may be used to group subexpressions.
- `numOps != 3 && (width > 10.0 || height > 10.0)`
- `!(width > 10.0 || height > 10.0)`

### Implicit Casting
There are several places where the value of an expression or subexpression will be implcitly cast to a boolean value. The unary-not operator (`!`) and the boolean binary operators (`&&`, `||`) cast the value of their subexpressions to boolean values before performing their operation. Additionally, the final result of evaluating an expression is always a boolean value, which may require casting if the expression represents a numeric or string value. Casting is performed by `VtValue::Cast`.

### Example
In the following example, a prim representing a light has an attribute that controls whether shadows are enabled (`enableShadows`). Two additional shadow-related attributes, `shadowColor` and `shadowMaxDistance`, are only displayed in the UI if the `enableShadows` attribute has a value of `1`.
```usda
def Scope "MyLight"
{
    custom int enableShadows = 0

    # Only show these if enableShadows is turned on
    custom color3d shadowColor = (0, 0, 0) (
        uiHints = {
            string shownIf = "enableShadows == 1"
        }
    )
    custom double shadowMaxDistance = -1.0 (
        uiHints = {
            string shownIf = "enableShadows == 1"
        }
    )
}
```

Alternatively, the shadow-related properties could be grouped into a namespace:
```usda
def Scope "MyLight"
{
    custom int shadow:enable = 0

    # Only show these if shadow:enable is turned on
    custom color3d shadow:color = (0, 0, 0) (
        uiHints = {
            string shownIf = "shadow:enable == 1"
        }
    )
    custom double shadow:maxDistance = -1.0 (
        uiHints = {
            string shownIf = "shadow:enable == 1"
        }
    )
}
```
