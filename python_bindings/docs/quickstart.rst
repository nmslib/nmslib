Python bindings for NMSLIB
====================================

Installation
------------

This project works with Python on version 2.7+ and 3.5+, and on Linux, OSX and the Windows operating systems. To install:

``pip install nmslib``

You may need to install Python dev-files. On Ubuntu, you can do it as follows:

``sudo apt-get install python3-dev``

Building on Windows requires Visual Studio 2015, see this project for more information.

Example Usage
-------------

.. code-block:: python

    import nmslib
    import numpy

    # create a random matrix to index
    data = numpy.random.randn(10000, 100).astype(numpy.float32)

    # initialize a new index, using a HNSW index on Cosine Similarity
    index = nmslib.init(method='hnsw', space='cosinesimil')
    index.addDataPointBatch(data)
    index.createIndex({'post': 2}, print_progress=True)

    # query for the nearest neighbours of the first datapoint
    ids, distances = index.knnQuery(data[0], k=10)

    # get all nearest neighbours for all the datapoint
    # using a pool of 4 threads to compute
    neighbours = index.knnQueryBatch(data, k=10, num_threads=4)

