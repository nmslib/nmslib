API Reference
=============

Information on the search methods (and their parameters) and spaces, can be found `on this page <https://github.com/nmslib/nmslib/tree/master/manual/README.md>`_.

nmslib.init
-----------

This function acts act the main entry point into NMS lib. This 
function should be called first before calling any other method.

.. autofunction:: nmslib.init


.. class:: nmslib.DistType

    .. attribute:: FLOAT
    .. attribute:: INT

.. class:: nmslib.DataType

    .. attribute:: DENSE_VECTOR
    .. attribute:: OBJECT_AS_STRING
    .. attribute:: SPARSE_VECTOR

nmslib.FloatIndex
-----------------

nmslib.dist.FloatIndex

.. autoclass:: nmslib.dist.FloatIndex
   :members:

nmslib.IntIndex
---------------

nmslib.dist.IntIndex

.. autoclass:: nmslib.dist.IntIndex
   :members:
