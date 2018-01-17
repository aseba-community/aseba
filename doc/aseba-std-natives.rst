Native functions standard library
=================================

``math.copy(A,B)``
-------------------

  Copy the array :math:`B` in the array :math:`A`, element by element: :math:`A_{i} = B_{i}`.

``math.fill(A,c)``
------------------
  Fill each element of the array :math:`A` with the constant :math:`c`: :math:`A_{i} = c`.

``math.addscalar(A, B, c)``
----------------------------
   Compute :math:`A_{i} = B_{i} + c` where :math:`c` is a scalar.


``math.add(A, B, C)``
---------------------
  Compute :math:`A_{i} = B_{i} + C_{i}` where :math:`A`, :math:`B` and :math:`C` are three arrays of the same size.


``math.sub(A, B, C)``
---------------------
   Compute :math:`A_{i} = B_{i} - C_{i}` where :math:`A`, :math:`B` and :math:`C` are three arrays of the same size.

``math.mul(A, B, C)``
---------------------
    Compute :math:`A_{i} = B_{i} \cdot C_{i}` where :math:`A`, :math:`B` and :math:`C` are three arrays of the same size.

    *Note that this is not a dot product.*

``math.div(A, B, C)``
---------------------
    Compute :math:`A_{i} = B_{i} / C_{i}` where :math:`A`, :math:`B` and :math:`C` are three arrays of the same size.

    *An exception will be triggered if a division by zero occurs.*

``math.min(A, B, C)``
---------------------
  Write the minimum of each element of :math:`B` and :math:`C` in
  :math:`A` where :math:`A`, :math:`B` and :math:`C` are three arrays of
  the same size: :math:`A_{i} = \mathrm{min}(B_{i}, C_{i})`.

``math.max(A, B, C)``
---------------------
  Write the maximum of each element of :math:`B` and :math:`C` in
  :math:`A` where :math:`A`, :math:`B` and :math:`C` are three arrays of
  the same size: :math:`A_{i} = \mathrm{max}(B_{i}, C_{i})`.

``math.clamp(A,B,C,D)``
-----------------------
  Clamp each element :math:`B_{i}` and store it in :math:`A_{i}` so that :math:`C_{i} < A_{i} < D_{i}`.


``math.dot(r, A, B, n)``
------------------------
   Compute the `dot product <http://en.wikipedia.org/wiki/Dot_product>`__
   between two arrays of the same size :math:`A` and
   :math:`B`:
   :math:`r = \frac{\sum_{i}(A_{i}\cdot B_{i})}{2^{n}}`


``math.stat(V, min, max, mean)``
--------------------------------
  Compute the minimum, the maximum and the mean values of array
  :math:`V`.

``math.argbounds(A, argmin, argmax)``
-------------------------------------
  Get the indices *argmin* and *argmax* corresponding to the minimum
  respectively maximum values of array :math:`A`.

``math.sort(A)``
----------------
  Sort the array :math:`A` in place.

``math.muldiv(A, B, C, D)``
---------------------------
  Compute multiplication-division using internal 32-bit precision:
  :math:`A_{i} = \frac{B_{i}\cdot C_{i}}{D_{i}}`.

  *An exception will be triggered if a division by zero occurs.*

``math.atan2(A, Y, X)`` [1]_
----------------------------
  Compute :math:`A_{i}=\arctan\left(\frac{Y_{i}}{X_{i}}\right)` using
  the signs of :math:`Y_{i}` and :math:`X_{i}` to determine the
  `quadrant <http://en.wikipedia.org/wiki/Quadrant_%28plane_geometry%29>`__
  of the output, where :math:`A`, :math:`Y` and :math:`X` are three
  arrays of the same size.

  *Note that* :math:`X_{i} = 0` *and* :math:`Y_{i} = 0` *will produce* :math:`A_{i} = 0`.

``math.sin(A, B)``  [1]_
------------------------
  Compute :math:`A_{i} = \sin(B_{i})` where :math:`A` and :math:`B` are
  two arrays of the same size.

``math.cos(A, B)`` [1]_
-----------------------
  Compute :math:`A_{i} = \cos(B_{i})` where :math:`A` and :math:`B` are
  two arrays of the same size.

``math.rot2(A, B, angle)`` [1]_
--------------------------------
  Rotate the array :math:`B` by *angle*, write the result to :math:`A`.

  *Note that* :math:`A` *and* :math:`B` *must both be arrays of size 2.*

``math.sqrt(A, B)``
-------------------
  Compute :math:`A_{i} = \sqrt{B_{i}}` where :math:`A` and :math:`B` are
  two arrays of the same size.

``math.rand(v)``
----------------
  Return a random value :math:`v` in the range :math:`-32768:32767`.

Since a scalar is considered to be an array of size one, you can use
these functions on scalars:

.. code-block::

    var theta = 16384
    var cos_theta
    call math.cos(cos_theta, theta)

.. [1] The trigonometric functions map the angles :math:`[-pi,pi[` radians to :math:`-32768,32767`.

   The resultant sin and cos values are similarly mapped, namely :math:`[-1.0,1.0[` to :math:`-32768,32767`.
