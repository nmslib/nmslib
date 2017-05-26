Python bindings for NMSLIB
=======


#### Installation

This project works with Python on version 2.7+ and 3.5+, and on
Linux/ OSX and Windows operating systems. To install first clone this repo,
and then run these commands from this directory:

```
pip install -r requirements.txt
python setup.py install
```

Building on Windows requires Visual Studio 2015, see this [project for more
information](https://github.com/pybind/python_example#installation).


#### Example Usage

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
