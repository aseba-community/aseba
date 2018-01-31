.. _aseba_natives:

Native functions standard library
=================================

Math
----

``math.copy(A,B)``
^^^^^^^^^^^^^^^^^^^

  Copy the array :math:`B` in the array :math:`A`, element by element: :math:`A_{i} = B_{i}`.

``math.fill(A,c)``
^^^^^^^^^^^^^^^^^^
  Fill each element of the array :math:`A` with the constant :math:`c`: :math:`A_{i} = c`.

``math.addscalar(A, B, c)``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
   Compute :math:`A_{i} = B_{i} + c` where :math:`c` is a scalar.


``math.add(A, B, C)``
^^^^^^^^^^^^^^^^^^^^^
  Compute :math:`A_{i} = B_{i} + C_{i}` where :math:`A`, :math:`B` and :math:`C` are three arrays of the same size.


``math.sub(A, B, C)``
^^^^^^^^^^^^^^^^^^^^^
   Compute :math:`A_{i} = B_{i} - C_{i}` where :math:`A`, :math:`B` and :math:`C` are three arrays of the same size.

``math.mul(A, B, C)``
^^^^^^^^^^^^^^^^^^^^^
    Compute :math:`A_{i} = B_{i} \cdot C_{i}` where :math:`A`, :math:`B` and :math:`C` are three arrays of the same size.

    *Note that this is not a dot product.*

``math.div(A, B, C)``
^^^^^^^^^^^^^^^^^^^^^
    Compute :math:`A_{i} = B_{i} / C_{i}` where :math:`A`, :math:`B` and :math:`C` are three arrays of the same size.

    *An exception will be triggered if a division by zero occurs.*

``math.min(A, B, C)``
^^^^^^^^^^^^^^^^^^^^^
  Write the minimum of each element of :math:`B` and :math:`C` in
  :math:`A` where :math:`A`, :math:`B` and :math:`C` are three arrays of
  the same size: :math:`A_{i} = \mathrm{min}(B_{i}, C_{i})`.

``math.max(A, B, C)``
^^^^^^^^^^^^^^^^^^^^^
  Write the maximum of each element of :math:`B` and :math:`C` in
  :math:`A` where :math:`A`, :math:`B` and :math:`C` are three arrays of
  the same size: :math:`A_{i} = \mathrm{max}(B_{i}, C_{i})`.

``math.clamp(A,B,C,D)``
^^^^^^^^^^^^^^^^^^^^^^^
  Clamp each element :math:`B_{i}` and store it in :math:`A_{i}` so that :math:`C_{i} < A_{i} < D_{i}`.


``math.dot(r, A, B, n)``
^^^^^^^^^^^^^^^^^^^^^^^^
   Compute the `dot product <http://en.wikipedia.org/wiki/Dot_product>`__
   between two arrays of the same size :math:`A` and
   :math:`B`:
   :math:`r = \frac{\sum_{i}(A_{i}\cdot B_{i})}{2^{n}}`


``math.stat(V, min, max, mean)``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  Compute the minimum, the maximum and the mean values of array
  :math:`V`.

``math.argbounds(A, argmin, argmax)``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  Get the indices *argmin* and *argmax* corresponding to the minimum
  respectively maximum values of array :math:`A`.

``math.sort(A)``
^^^^^^^^^^^^^^^^
  Sort the array :math:`A` in place.

``math.muldiv(A, B, C, D)``
^^^^^^^^^^^^^^^^^^^^^^^^^^^
  Compute multiplication-division using internal 32-bit precision:
  :math:`A_{i} = \frac{B_{i}\cdot C_{i}}{D_{i}}`.

  *An exception will be triggered if a division by zero occurs.*

``math.atan2(A, Y, X)`` [1]_
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  Compute :math:`A_{i}=\arctan\left(\frac{Y_{i}}{X_{i}}\right)` using
  the signs of :math:`Y_{i}` and :math:`X_{i}` to determine the
  `quadrant <http://en.wikipedia.org/wiki/Quadrant_%28plane_geometry%29>`__
  of the output, where :math:`A`, :math:`Y` and :math:`X` are three
  arrays of the same size.

  *Note that* :math:`X_{i} = 0` *and* :math:`Y_{i} = 0` *will produce* :math:`A_{i} = 0`.

``math.sin(A, B)``  [1]_
^^^^^^^^^^^^^^^^^^^^^^^^
  Compute :math:`A_{i} = \sin(B_{i})` where :math:`A` and :math:`B` are
  two arrays of the same size.

``math.cos(A, B)`` [1]_
^^^^^^^^^^^^^^^^^^^^^^^
  Compute :math:`A_{i} = \cos(B_{i})` where :math:`A` and :math:`B` are
  two arrays of the same size.

``math.rot2(A, B, angle)`` [1]_
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  Rotate the array :math:`B` by *angle*, write the result to :math:`A`.

  *Note that* :math:`A` *and* :math:`B` *must both be arrays of size 2.*

``math.sqrt(A, B)``
^^^^^^^^^^^^^^^^^^^
  Compute :math:`A_{i} = \sqrt{B_{i}}` where :math:`A` and :math:`B` are
  two arrays of the same size.

``math.rand(v)``
^^^^^^^^^^^^^^^^
  Return a random value :math:`v` in the range :math:`-32768:32767`.

Since a scalar is considered to be an array of size one, you can use
these functions on scalars:

::

    var theta = 16384
    var cos_theta
    call math.cos(cos_theta, theta)

Double-ended queues
-------------------

The `Deque` native library provides functions for `double-ended queue <https://en.wikipedia.org/wiki/Double-ended_queue>`_ operations in an object-oriented style on specially-formatted arrays.
The array for a `deque` object must be of size :math:`$2 + m \cdot k\,\,$` where :math:`$k$` is the size of the tuples [2]_ in the deque, and :math:`$m$` is the maximum number of tuples to be stored.

An `index` :math:`$i$` into a deque is between two elements: the integer :math:`$i$` counts the number of elements to the left of the index.

:code:`deque.size(Q,n)`
^^^^^^^^^^^^^^^^^^^^^^^

Set :math:`$n$` to the number of elements in deque :math:`$Q$`. If :math:`$n=0$` then :math:`$Q$` is empty. Note that :math:`$n$` must be divided by the tuple size to obtain the number of tuples in :math:`$Q$`.

:code:`deque.push_front(Q,A)`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Insert tuple :math:`$A$` before the first tuple of deque :math:`$Q$`.

:code:`deque.push_back(Q,A)`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Insert tuple :math:`$A$` after the last tuple in deque :math:`$Q$`.

:code:`deque.pop_front(Q,A)`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Remove the first :math:`length($A$)` elements of deque :math:`$Q$` and place them in tuple :math:`$A$`.

:code:`deque.pop_back(Q,A)`
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Remove the last :math:`length($A$)` elements of deque :math:`$Q$` and place them in tuple :math:`$A$`.

:code:`deque.get(Q,A,i)`
^^^^^^^^^^^^^^^^^^^^^^^^
Copy into tuple :math:`$A$` , :math:`length($A$)` elements from deque :math:`$Q$` starting from index :math:`$i$`.

:code:`deque.set(Q,A,i)`
^^^^^^^^^^^^^^^^^^^^^^^^
Copy into deque :math:`$Q$` starting at index :math:`$i$`,  :math:`length($A$)` elements from tuple :math:`$A$`.

:code:`deque.insert(Q,A,i)`
^^^^^^^^^^^^^^^^^^^^^^^^^^^
Shift right the suffix of deque :math:`$Q$` starting at index :math:`$i$` by :math:`length($A$)` elements, then copy tuple :math:`$A$` into the deque :math:`$Q$` at that index.

:code:`deque.erase(Q,i,k)`
^^^^^^^^^^^^^^^^^^^^^^^^^^
Erase :math:`$k$` elements from deque :math:`$Q$` at index :math:`$i$` by shifting the suffix starting from :math:`$i+k$` left. Length :math:`$k$` should be the tuple size or a multiple.

Here is a simple motion queue, that accepts `operations` defined by a time and motor speeds, and executes them first-in, first-out.
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
.. code::

    var operation[3] # Tuple of size 3
    var Queue[2 + (3*40)] # Store up to 40 operation tuples
    var n

    sub motion_add
        call deque.push_back(Queue, event.args[0:2])

    onevent timer0
        call deque.size(Queue, n)
        if n > 0 then
            call deque.pop_front(Queue, operation)
            timer.period[0] = operation[0]
            motor.left.target = operation[1]
            motor.right.target = operation[2]
        end


.. [1] The trigonometric functions map the angles :math:`[-pi,pi[` radians to :math:`-32768,32767`.
   The resultant sin and cos values are similarly mapped, namely :math:`[-1.0,1.0[` to :math:`-32768,32767`.

.. [2] A `tuple` is simply a small array of values that are inserted in the `deque` together
