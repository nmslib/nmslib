Python bindings for NMSLIB
=======


#### Installation

This project works with Python on version 2.7+ and 3.5+, and on
Linux, OSX and the Windows operating systems. To install:

```
pip install nmslib
```

You may need to install Python dev-files. On Ubuntu, you can do it as follows:

```
sudo apt-get install python3-dev
```

Building on Windows requires Visual Studio 2015, see this [project for more
information](https://github.com/pybind/python_example#installation).


#### Example Usage

Here is a simple example, but we also have [Python notebooks with more elaborate end-to-end examples, which include even computation of gold-standard data](notebooks) (for both sparse and dense space):

```python
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
```

#### Basic tuning guidelines

The basic parameter tuning/selection guidelines are available [here](/python_bindings/parameters.md).

#### Logging

NMSLIB produces quite a few informational messages. By default, they are not shown in Python. To enable debugging, one should use the following commands **before** importing the library:

```
import logging
logging.basicConfig(level=logging.DEBUG)
```

#### Installing with Extras

To enable extra methods like those provided by FALCONN and LSHKIT you need to follow an extra couple steps.

These methods require a development version of the
following libraries: Boost, GNU scientific library, and Eigen3. To install on Ubuntu:

```
sudo apt-get install libboost-all-dev libgsl0-dev libeigen3-dev
```

Next clone the repository and build with the C++ files using CMake:

```
cd similarity_search
cmake . -DWITH_EXTRAS=1
make
cd ..
```

Finally build and install the python extension:

```
cd python_bindings
pip install -r requirements.txt
python setup.py install
```

#### Additional documentation

[More detailed documentation is also available](https://nmslib.github.io/nmslib/) (thanks to Ben Frederickson).
