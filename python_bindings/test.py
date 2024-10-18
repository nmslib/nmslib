import nmslib
import numpy
import time

# create a random matrix to index
data = numpy.random.randn(10000, 256).astype(numpy.float32)

# initialize a new index, using a HNSW index on Cosine Similarity
index = nmslib.init(method='hnsw', space='cosinesimil')
start = time.time()
index.addDataPointBatch(data)
print('Time to add data: %f' % (time.time() - start))
start = time.time()
index.createIndex({'post': 2}, print_progress=True)
print('Time to create index: %f' % (time.time() - start))

# query for the nearest neighbours of the first datapoint
ids, distances = index.knnQuery(data[0], k=10)

# get all nearest neighbours for all the datapoint
# using a pool of 4 threads to compute
start = time.time()
neighbours = index.knnQueryBatch(data, k=10, num_threads=4)
print('Time to query data: %f' % (time.time() - start))
print(numpy.array(neighbours))