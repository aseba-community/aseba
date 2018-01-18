The Aseba language
==================

The Aseba language syntax resembles that of a popular class of
programming languages, including
`Pascal <http://en.wikipedia.org/wiki/Pascal_%28programming_language%29>`__
and `Matlab <http://en.wikipedia.org/wiki/Matlab>`__ (a common
scientific programming language) for instance; we expect this similarity
to allow developers with previous knowledge of any of these languages to
feel quickly at ease with Aseba and thus lower the learning curve.
Semantically, it is a simple `imperative programming
language <http://en.wikipedia.org/wiki/Imperative_programming>`__ with a
single basic `data type <http://en.wikipedia.org/wiki/Data_type>`__ (16
bit
`signed <http://en.wikipedia.org/wiki/Signed_number_representations>`__
`integers <http://en.wikipedia.org/wiki/Integer_%28computer_science%29>`__)
and `arrays <http://en.wikipedia.org/wiki/Array_data_type>`__. This
simplicity allows developers to program behaviours with no prior
knowledge of a `type
system <http://en.wikipedia.org/wiki/Type_system>`__, integers being the
most natural type of variables and well suited to programming
`microcontroller <http://en.wikipedia.org/wiki/Microcontroller>`__-based
mobile robots.

Comments
--------

Comments allow the adding of information which is ignored by the
`compiler <http://en.wikipedia.org/wiki/Compiler>`__. Comments are very
useful to annotate the code with human-readable notes or to temporarily
disable some code. Comments begin with a *#* and terminate at the end of
the line.

Example:

::

    # this is a comment
    var b # another comment

Comments spanning several lines are also possible. They begin with *#\**
and end with *\*#*. Example:

::

    #*
    this is a comment spanning
    several lines
    *#
    var b # a simple comment

Scalars
-------

`Scalar values <http://en.wikipedia.org/wiki/Scalar_%28computing%29>`__
are used in Aseba to represent numbers. They can be used in any
expressions, like the initialisation of a variable, a mathematical
expression or in a logical condition. The values are comprised between
-32768 and 32767, which is the range of 16 bit signed integers.

Notation
~~~~~~~~

Scalars can be given using several
`radices <http://en.wikipedia.org/wiki/Radix>`__. The most natural way
is the `decimal system <http://en.wikipedia.org/wiki/Decimal>`__, using
digits from 0 to 9. Negative numbers are declared using a ``-`` (minus)
sign preceding the number.

::

    i = 42
    i = 31415
    i = -7

Both `binary <http://en.wikipedia.org/wiki/Binary_numeral_system>`__ and
`hexadecimal <http://en.wikipedia.org/wiki/Hexadecimal>`__ numbers can
also be used. Binary numbers are prefixed by ``0b``, whereas hexadecimal
numbers are prefixed by ``0x``.

::

    # binary notation
    i = 0b110         # i = 6
    i = 0b11111111    # i = 255

    # hexadecimal notation
    i = 0x10    # i = 16
    i = 0xff    # i = 255

In binary notation, values are comprised between ``0b0000000000000000``
and ``0b1111111111111111``, while in hexadecimal they are comprised
between ``0x0`` and ``0xffff``. Values that would be over 32767 in
decimal are interpreted as negative numbers.

Variables
~~~~~~~~~

Variables refer either to single
`scalar <http://en.wikipedia.org/wiki/Scalar_%28computing%29>`__ values
or to arrays of scalar values. You must declare all user-defined
variables using the keyword ``var`` at the beginning of the Aseba
programme before doing any processing.

The name of a variable is defined by these rules:

-  The name can only contain upper or lower case alphanumeric
   characters, '\_' or '.'
-  The name must start with a valid alphabetic character or '\_'
-  The name is case sensitive: a variable named "foo" is different from
   one named "Foo"
-  The name cannot be identical with one of Aseba's keywords (see
   `reserved keywords <Reserved keywords_>`_)

Variables may be initialised in the declaration, using the assignment
symbol and combined with any valid mathematical expression. A variable
without any prior initialisation may have a random value, it should
never be assumed to be zero.

Example:

::

    var a
    var b = 0
    var c = 2*a + b        # warning: 'a' is not initialised

Reserved keywords
~~~~~~~~~~~~~~~~~


The following keywords cannot be used as valid names for variables, as
they are already used by the Aseba language.

 - `abs`
 - `call`
 - `callsub`
 - `do`
 - `else`
 - `elseif`
 - `emit`
 - `end`
 - `for`
 - `if`
 - `in`
 - `onevent`
 - `return`
 - `step`
 - `sub`
 - `then`
 - `var`
 - `when`
 - `while`

Constants
~~~~~~~~~

`Constants <http://en.wikipedia.org/wiki/Constant_%28programming%29>`__
can be defined in Aseba Studio using the "Constants" panel, but they
cannot be defined directly in the code. A constant represents a numeric
value which can be used wherever a number can be used. But unlike a
variable, a constant cannot be modified during execution. Constants are
useful when you want to easily change the behaviour between different
executions, such as to adapt a threshold value, with a scope spanning
several Aseba nodes. A constant cannot have the same name as a variable,
otherwise an error is raised. By convention, a constant is often written
in upper case.

::

    # assuming a constant named THRESHOLD
    var i = 600

    if i > THRESHOLD then
        i = THRESHOLD - 1
    end

Arrays
~~~~~~

Arrays represent a contiguous area in memory, addressed as a single
logical entity. The size of an array is fixed and must be specified in
the declaration. Arrays can be declared using the usual `square bracket
operator <http://en.wikipedia.org/wiki/Bracket#Uses_of_.22.5B.22_and_.22.5D.22>`__
``[]``. The number between the square brackets specifies the number of
elements to be assigned to the array, thereafter referred to as its
size. It can be any constant expression, including mathematical
operations using scalars and constants. An optional assignment can be
made using the array constructor (see below). If this is done, the size
of the array need not be specified.

Example:

::

    var a[10]              # array of 10 elements
    var b[3] = [2,3,4]     # initialisation
    var c[] = [3,1,4,1,5]  # implicit size of 5 elements
    var d[3*FOO-1]         # size declared using a constant expression (FOO is a constant)

Arrays can be accessed in several ways:

-  A single element is accessed by using the square bracket operator
   with a single value. Array indexes begin at zero. Any expression can
   be used as index, including mathematical expressions involving other
   variables.
-  A range of elements can be accessed by using the square bracket
   operator with two constant expressions separated by a colon ':'. The
   validity of the range is checked at compile-time.
-  If the square brackets are omitted, the entire array is accessed.

Example:

::

    var foo[5] = [1,2,3,4,5]
    var i = 1
    var a
    var b[3]
    var c[5]
    var d[5]

    a = foo[0]        # copy first element from 'foo' to 'a'
    a = foo[2*i-2]    # same
    b = foo[1:3]      # take 2nd, 3rd and 4th elements of 'foo', copy to 'b'
    b = foo[1:2*2-1]  # same
    c = foo           # copy 5 elements from array 'foo' to array 'c'
    d = c * foo       # multiply arrays 'foo' and 'c' element by element, copy result to 'd'

A scalar variable is considered to be an array of size one so the
following code is legal:

::

    var a[1] = [7]
    var b = 0
    b = a

Array constructors
------------------

Array constructors are a way to build arrays from variables, other
arrays, scalars, or even complex expressions. They are useful in several
cases, for example when initialising another array, or as operands in
expressions, functions and events. An array constructor is made by using
square brackets enclosing several expressions separated by a ``,``
(comma). The size of an array constructor is the sum of the sizes of the
individual elements, and it must match the size of the array in which
the result is stored.

Example:

::

    var a[5] = [1,2,3,4,5]  # array constructor to initialise an array
    var b[3] = [a[1:2],0]   # results in array b initialised to [2,3,0]
    a = a + [1,1,1,1,1]     # add 1 to each element of array a
    a = [b[1]+2,a[0:3]]     # results in [5,2,3,4,5]

Expressions and assignments
---------------------------

Expressions allow mathematical computations and are written using the
normal mathematical
`infix <http://http://en.wikipedia.org/wiki/Infix_notation>`__ syntax.
Assignments use the keyword ``=`` and set the result of the computation
of an expression into a scalar variable, an array element or a whole
array, depending on the size of the operands. Aseba provides several
operators. Please refer to the table below for a brief description, as
well as for the precedence of each operator. To evaluate an expression
in a different order, pairs of parentheses can be used to group
sub-expressions.

+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
| `Precedence <http://en.wikipedia.org/wiki/Order_of_operations>`__   | `Operator <http://en.wikipedia.org/wiki/Operator_%28programming%29>`__   | Description                                                                                            | `Associativity <http://en.wikipedia.org/wiki/Associativity>`__      | `Arity <http://en.wikipedia.org/wiki/Arity>`__   |
+=====================================================================+==========================================================================+========================================================================================================+=====================================================================+==================================================+
| 1                                                                   | ()                                                                       | Group a sub-expression                                                                                 |                                                                     | unary                                            |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
|                                                                     | []                                                                       | Index an array                                                                                         |                                                                     | unary                                            |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
|                                                                     | -                                                                        | Unary minus                                                                                            |                                                                     | unary                                            |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
|                                                                     | ~                                                                        | `Binary not <http://en.wikipedia.org/wiki/Bitwise_operation#NOT>`__                                    |                                                                     | unary                                            |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
|                                                                     | abs                                                                      | `Absolute value <http://en.wikipedia.org/wiki/Absolute_value>`__                                       |                                                                     | unary                                            |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
| 2                                                                   | \* /                                                                     | Multiplication, division                                                                               |                                                                     | binary                                           |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
|                                                                     | %                                                                        | `Modulo <http://en.wikipedia.org/wiki/Modulo_operation>`__                                             |                                                                     | binary                                           |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
| 3                                                                   | + -                                                                      | Addition, subtraction                                                                                  |                                                                     | binary                                           |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
| 4                                                                   | << >>                                                                    | `Left shift, right shift <http://en.wikipedia.org/wiki/Bitwise_operation#Arithmetic_shift>`__          |                                                                     | binary                                           |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
| 5                                                                   | &                                                                        | `Binary and <http://en.wikipedia.org/wiki/Bitwise_operation#AND>`__                                    | Left associative                                                    | binary                                           |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
| 6                                                                   | ^                                                                        | `Binary exclusive or (xor) <http://en.wikipedia.org/wiki/Bitwise_operation#XOR>`__                     | Left associative                                                    | binary                                           |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
| 7                                                                   |                                                                          | `Binary or <http://en.wikipedia.org/wiki/Bitwise_operation#OR>`__                                      | Left associative                                                    |                                                  |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
| 8                                                                   | == != < <= > >=                                                          | Condition                                                                                              |                                                                     | binary                                           |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
| 9                                                                   | not                                                                      | `Logical not <http://en.wikipedia.org/wiki/Logical_negation>`__ †                                      |                                                                     | unary                                            |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
| 10                                                                  | and                                                                      | `Logical and <http://en.wikipedia.org/wiki/Logical_conjunction>`__ †                                   |                                                                     | binary                                           |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
| 11                                                                  | or                                                                       | `Logical or <http://en.wikipedia.org/wiki/Logical_disjunction>`__ †                                    |                                                                     | binary                                           |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
| 12                                                                  | =                                                                        | Assignment                                                                                             |                                                                     | binary                                           |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
|                                                                     | ^= &=                                                                    | Assignment by binary or, xor, and                                                                      |                                                                     | fop                                              |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
|                                                                     | \*= /=                                                                   | Assignment by product and quotient                                                                     |                                                                     | binary                                           |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
|                                                                     | %=                                                                       | Assignment by modulo                                                                                   |                                                                     | binary                                           |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
|                                                                     | += -=                                                                    | Assignment by sum and difference                                                                       |                                                                     | binary                                           |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
|                                                                     | <<= >>=                                                                  | Assignment by left / right shift                                                                       |                                                                     | binary                                           |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+
|                                                                     | ++ --                                                                    | `Unary increment and decrement <http://en.wikipedia.org/wiki/Increment_and_decrement_operators>`__     |                                                                     | unary                                            |
+---------------------------------------------------------------------+--------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------+--------------------------------------------------+

*Footnotes* † Only available from within ``if``, ``when``, and ``while``
structures ‡ Only available as statements, such as ``a--`` or
``a[i]++``, not within an expression

The *assignment by* versions of the binary operators work by applying
the operator to a variable and storing the result in this same variable.
For instance, ``A *= 2`` is equal to ``A = A * 2``. These short-cuts aim
at making the code more readable.

Example:

::

    a = 1 + 1
    # Result: a = 2
    a *= 3
    # Result: a = 6
    a++
    # Result: a = 7

    b = b + d[0]
    b = (a - 7) % 5
    c[a] = d[a]
    c[0:1] = d[2:3] * [3,2]

Usage
~~~~~

Mathematical expressions are a general tool. As such, they can be used
in a great variety of situations. Just to mention a few:

-  On the right side of an assignment
-  As an index when accessing elements of arrays
-  Inside function calls
-  As argument when emitting an event

Flow control
------------

Conditionals
~~~~~~~~~~~~

Aseba provides two types of `conditionals
statements <http://en.wikipedia.org/wiki/Conditional_%28programming%29>`__:
``if``-statements and ``when``-statements. A conditional statement
consists of a conditional expression and blocks of code. Conditional
expressions are formed from a comparison
`operator <http://en.wikipedia.org/wiki/Relational_operator>`__ and two
`operands <http://en.wikipedia.org/wiki/Operand>`__ which are arithmetic
expressions; for example, ``a < b+3`` is a conditional expression. The
following table lists the comparison operators:

+-------------------------------------------------------------------+-----------------------------------------------------------------+
| `Operator <http://en.wikipedia.org/wiki/Relational_operator>`__   | Truth value                                                     |
+===================================================================+=================================================================+
| ``==``                                                            | true if operands are equal                                      |
+-------------------------------------------------------------------+-----------------------------------------------------------------+
| ``!=``                                                            | true if operands are different                                  |
+-------------------------------------------------------------------+-----------------------------------------------------------------+
| ``>``                                                             | true if first operand is strictly larger than the second one    |
+-------------------------------------------------------------------+-----------------------------------------------------------------+
| ``>=``                                                            | true if the operand is larger or equal to the second one        |
+-------------------------------------------------------------------+-----------------------------------------------------------------+
| ``<``                                                             | true if first operand is strictly smaller than the second one   |
+-------------------------------------------------------------------+-----------------------------------------------------------------+
| ``<=``                                                            | true if the operand is smaller or equal to the second one       |
+-------------------------------------------------------------------+-----------------------------------------------------------------+

A conditional expression may also be formed by combining comparison
expressions with the logical operators ``and`` (`logical
conjunction <http://en.wikipedia.org/wiki/Logical_conjunction>`__),
``or`` (`logical
disjunction <http://en.wikipedia.org/wiki/Logical_disjunction>`__) and
``not`` (`logical
negation <http://en.wikipedia.org/wiki/Logical_negation>`__); for
example, ``(a < b+3) or (a < 0)``. Precedence can be controlled by
parentheses; for example
``((a < b) or (b < c)) and ((d < e) or (e < f))``. While the Aseba
language does not have boolean variables or literals — so you cannot
write ``flag = true`` or ``if flag then`` — the result of a comparison
is considered to be a boolean value (true or false) that can be used
with the logical operators. Conditional expressions are also used in
``while``-statements (see section `loops <#toc12>`__).

Both ``if`` and ``when`` execute a different block of code according to
whether a condition is true or false; but ``when`` executes the block
corresponding to true only if the previous evaluation of the condition
was false and the current one is true. This allows the execution of code
only when something changes. The ``if`` conditional executes a first
block of code if the condition is true, a second block of code to
execute if the condition is false can be added using the ``else``
keyword. Furthermore, additional conditions can be chained using the
``elseif`` keyword.

Example:

::

    if a - b > c[0] then
        c[0] = a
    elseif a > 0 then
        b = a
    else
        b = 0
    end

    if a < 2 and a > 2 then
        b = 1
    else
        b = 0
    end

    when a > b do
        leds[0] = 1
    end

Here the ``when`` block executes only when ``a`` *becomes* larger than
``b``. ### Loops

Two constructs allow the creation of loops: ``while`` and ``for``.

A ``while`` loop repeatedly executes a block of code as long as the
condition is true. The condition is of the same form as the one ``if``
uses.

Example:

::

    while i < 10 do
        v = v + i * i
        i = i + 1
    end

A ``for`` loop allows a variable to
`iterate <http://en.wikipedia.org/wiki/Iterate>`__ over a range of
integers, with an optional step size.

Example:

::

    for i in 1:10 do
        v = v + i * i
    end
    for i in 30:1 step -3 do
        v = v - i * i
    end

The value of the loop variable is undefined after the execution of the
loop. It will usually be the last value + step, but can take another
value due to optimisations, for instance in single-element loops.

Blocks
------

Subroutines
~~~~~~~~~~~

When you want to perform the same sequence of operations at two or more
different places in the code, you can write common code just once in a
subroutine and then call this subroutine from different places. You
define a subroutine using the ``sub`` keyword followed by the name of
the subroutine. You call the subroutine using the ``callsub`` keyword,
followed by the name of the subroutine. Subroutines cannot have
arguments, nor be
`recursive <http://en.wikipedia.org/wiki/Recursion_%28computer_science%29>`__,
either directly or indirectly. Subroutines can access any variable.

Example:

::

    var v = 0

    sub toto
    v = 1

    onevent test
    callsub toto

Events
------

Aseba is an `event-based
architecture <http://en.wikipedia.org/wiki/Event-driven_programming>`__,
which means that events trigger code execution asynchronously. Events
can be external, for instance a user-defined event coming from another
Aseba node, or internal, for instance emitted by a sensor that provides
updated data. The reception of an event executes, if defined, the block
of code that begins with the ``onevent`` keyword followed by the name of
the event; the code at the top of the programme is executed when the
programme is started or reset.

To allow the execution of related code upon new events, programmes must
not block and thus must not contain any infinite loop. For instance in
the context of robotics, where a traditional robot control programme
would do some processing inside an infinite loop, an Aseba programme
would just do the processing inside a sensor-related event.

Example:

::

    var run = 0

    onevent start
    run = 1

    onevent stop
    run = 0

Return Statement
----------------

It is possible to early return from subroutines and stop the execution
of events with the ``return`` statement.

Example:

::

    var v = 0

    sub toto
    if v == 0 then
        return
    end
    v = 1

    onevent test
    callsub toto
    return
    v = 2

Initialization
~~~~~~~~~~~~~~

Statements placed between the variable declarations and the subroutines
and event handlers are run when the program is initialized:

::

    var state

    state = 0
    call leds.bottom.left(0,0,32)
    call leds.bottom.right(0,32,0)
    call leds.top(32,0,0)

While the initialization of ``state`` could have been done in its
declaration, the initialization of the leds must be done by statements.
When programming a robot, you will usually want to define some event
that will re-initialize the state of the robot. This is possible by
writing the statements within a subroutine and calling it from the event
handler. It is also possible to call the subroutine as part of the
program initialization even though it has not yet been declared:

::

    var state

    callsub init  # Initialize the program

    # Subroutine for initialization
    sub init
        state = 0
        call leds.bottom.left(0,0,32)
        call leds.bottom.right(0,32,0)
        call leds.top(32,0,0)

    # Re-initialize when center button is touched
    onevent button.center
        callsub init

Sending external events
-----------------------

The programme can send external events by using the ``emit`` keyword,
followed by the name of the event and the name of the variable to send,
if any. If a variable is provided, the size of the event must match the
size of the
`argument <http://en.wikipedia.org/wiki/Argument_%28computer_science%29>`__
to be emitted. Instead of a variable, array constructors and
mathematical expressions can also be used in more complex situations.
Events allow the programme to trigger the execution of code on another
node or to communicate with an external programme.

::

    onevent ir_sensors
    emit sensors_values proximity_sensors_values

Native functions
----------------

We designed the Aseba language to be simple in order to allow a quick
understanding even by novice developers and to implement the virtual
machine efficiently on a micro-controller. To perform complex or
resource-intensive processing, we provide native functions that are
implemented in native code for efficiency. For instance, a native
function is the natural way to implement a scalar product.

Native functions are safe, as they specify and check the size of their
arguments, which can be constants, variables, array accesses, array
constructors and expressions. In the case of an array, you can access
the whole array, a single element, or a sub-range of the array. Native
functions take their arguments by
`reference <http://en.wikipedia.org/wiki/Call_by_reference#Call_by_reference>`__
and can modify their contents but do not return any value. You can use
native functions through the ``call`` keyword.

Example:

::

    var a[3] = 1, 2, 3
    var b[3] = 2, 3, 4
    var c[5] = 5, 10, 15
    var d
    call math.dot(d, a, b, 3)
    call math.dot(d, a, c[0:2], 3)
    call math.dot(a[0], c[0:2], 3)

What to read next?
------------------

You might be interested to read:

-  `Description of the native functions standard library <std-natives.rst>`__
