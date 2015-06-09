import sklearn.neighbors
import annoy
import pyflann
import panns
import nearpy, nearpy.hashes, nearpy.distances
import pykgraph
import nmslib
import gzip, numpy, time, os, multiprocessing
try:
    from urllib import urlretrieve
except ImportError:
    from urllib.request import urlretrieve # Python 3
import sklearn.cross_validation, sklearn.preprocessing, random

class BaseANN(object):
    pass

class FLANN(BaseANN):
    def __init__(self, target_precision):
        self._target_precision = target_precision
        self.name = 'FLANN(target_precision=%f)' % target_precision

    def fit(self, X):
        self._flann = pyflann.FLANN(target_precision=self._target_precision, algorithm='autotuned', log_level='info')
        X = sklearn.preprocessing.normalize(X, axis=1, norm='l2')
        self._flann.build_index(X)

    def query(self, v, n):
        v = sklearn.preprocessing.normalize(v, axis=1, norm='l2')[0]
        return self._flann.nn_index(v, n)[0][0]

    def freeIndex(self):
        pass


class KGraph(BaseANN):
    def __init__(self, P):
        self.name = 'KGraph(P=%d)' % P
        self._P = P

    def fit(self, X):
        X = sklearn.preprocessing.normalize(X, axis=1, norm='l2')
        self._kgraph = pykgraph.KGraph()
        self._kgraph.build(X, iterations=30, L=100, delta=0.002, recall=0.99, K=25)
        self._X = X # ???

    def query(self, v, n):
        v = sklearn.preprocessing.normalize(v, axis=1, norm='l2')[0]
        result = self._kgraph.search(self._X, numpy.array([v]), K=n, threads=1, P=self._P)
        return result[0]

    def freeIndex(self):
        pass

class Annoy(BaseANN):
    def __init__(self, n_trees, n_candidates):
        self._n_trees = n_trees
        self._n_candidates = n_candidates
        self.name = 'Annoy(n_trees=%d, n_cand=%d)' % (n_trees, n_candidates)

    def fit(self, X):
        self._annoy = annoy.AnnoyIndex(f=X.shape[1], metric='angular')
        for i, x in enumerate(X):
            self._annoy.add_item(i, x.tolist())
        self._annoy.build(self._n_trees)

    def query(self, v, n):
        return self._annoy.get_nns_by_vector(v.tolist(), self._n_candidates)[:n]

    def freeIndex(self):
        pass

class BruteForce(BaseANN):
    def __init__(self):
        self.name = 'BruteForce()'

    def fit(self, X):
        self._nbrs = sklearn.neighbors.NearestNeighbors(algorithm='brute', metric='cosine')
        self._nbrs.fit(X)

    def query(self, v, n):
        return list(self._nbrs.kneighbors(v, return_distance=False, n_neighbors=n)[0])

    def freeIndex(self):
        pass

class Nmslib(BaseANN):
    def __init__(self, method_name, method_param):
        self._method_name = method_name
        self._method_param = method_param
        self.name = 'Nmslib(method_name=%s, method_param=%s)' % (method_name, method_param)

    def fit(self, X):
        dist = 'cosinesimil'
        self._index = nmslib.initIndex(X.shape[0], dist, [], self._method_name, self._method_param, nmslib.DataType.VECTOR, nmslib.DistType.FLOAT)
	
        for i, x in enumerate(X):
            nmslib.setData(self._index, i, x.tolist())
        nmslib.buildIndex(self._index)

    def query(self, v, n):
        return nmslib.knnQuery(self._index, n, v.tolist())

    def freeIndex(self):
        nmslib.freeIndex(self._index)

class NmslibMPLSH(BaseANN):
    def __init__(self, method_param):
        self._method_name = 'lsh_multiprobe'
        self._method_param = method_param
        self.name = 'Nmslib(method_name=%s, method_param=%s)' % (self._method_name, method_param)

    def fit(self, X):
        dist = 'l2'
        self._index = nmslib.initIndex(X.shape[0], dist, [], self._method_name, self._method_param, nmslib.DataType.VECTOR, nmslib.DistType.FLOAT)
	
        for i, x in enumerate(X):
            nmslib.setData(self._index, i, x.tolist())
        nmslib.buildIndex(self._index)

    def query(self, v, n):
        return nmslib.knnQuery(self._index, n, v.tolist())

    def freeIndex(self):
        pass

class BruteForceNmslib(BaseANN):
    def __init__(self, copyMem):
        self.name = 'BruteForce (nmslib)'
        self._method_name = 'seq_search'
        self._method_param = ['copyMem=' + str(copyMem)]

    def fit(self, X):
        dist = 'cosinesimil'
        self._index = nmslib.initIndex(X.shape[0], dist, [], self._method_name, self._method_param, nmslib.DataType.VECTOR, nmslib.DistType.FLOAT)

        for i, x in enumerate(X):
            nmslib.setData(self._index, i, x.tolist())
        nmslib.buildIndex(self._index)

    def query(self, v, n):
        return nmslib.knnQuery(self._index, n, v.tolist())

    def freeIndex(self):
        pass

def get_dataset(which):
    local_fn = os.path.join('install', which + '.txt')
    f = open(local_fn)

    X = []
    for i, line in enumerate(f):
        v = [float(x) for x in line.strip().split()]
        X.append(v)
        #if len(X) == 100: # just for debugging purposes right now
        #    break

    X = numpy.vstack(X)
    X_train, X_test = sklearn.cross_validation.train_test_split(X, test_size=1000, random_state=42)
    print X_train.shape, X_test.shape
    return X_train, X_test

def run_algo(library, algo):
    t0 = time.time()
    if algo != 'bf':
        algo.fit(X_train)
    build_time = time.time() - t0

    #for i in xrange(3): # Do multiple times to warm up page cache
    for i in xrange(1): # Disable multiple runs for now
        t0 = time.time()
        k = 0.0
        for v, correct in queries:
            found = algo.query(v, 10)
            k += len(set(found).intersection(correct))
        search_time = (time.time() - t0) / len(queries)
        precision = k / (len(queries) * 10)

        output = [library, algo.name, build_time, search_time, precision]
        print output

    # This can be important!
    algo.freeIndex()

    f = open('data.tsv', 'a')
    f.write('\t'.join(map(str, output)) + '\n')
    f.close()

bfSklearn = BruteForce()
bf = BruteForceNmslib(1)

algos = {
    #'bruteforce(sklearn)': [bfSklearn],
    #'bruteforce(nmslib)': [BruteForceNmslib(0)],
    #'bruteforce(nmslib)': [BruteForceNmslib(1)],
    #'flann': [FLANN(0.2), FLANN(0.5), FLANN(0.7), FLANN(0.8), FLANN(0.9), FLANN(0.95), FLANN(0.97), FLANN(0.98), FLANN(0.99), FLANN(0.995)],
    #'annoy': [Annoy(3, 10), Annoy(5, 25), Annoy(10, 10), Annoy(10, 40), Annoy(10, 100), Annoy(10, 200), Annoy(10, 400), Annoy(10, 1000), Annoy(20, 20), Annoy(20, 100), Annoy(20, 200), Annoy(20, 400), Annoy(40, 40), Annoy(40, 100), Annoy(40, 400), Annoy(100, 100), Annoy(100, 200), Annoy(100, 400), Annoy(100, 1000)],
    #'kgraph': [KGraph(2), KGraph(5), KGraph(10), KGraph(20), KGraph(50), KGraph(100), KGraph(200), KGraph(500), KGraph(1000), KGraph(2000)],

    # MP-lsh isn't doing a great job, because it's not designed for the cosine similarity
    #'MP-lsh(nmslib)':[
                #NmslibMPLSH(['desiredRecall=0.99','H=1200001','T=10','L=50','tuneK=10']),
                #NmslibMPLSH(['desiredRecall=0.97','H=1200001','T=10','L=50','tuneK=10']),
                #NmslibMPLSH(['desiredRecall=0.95','H=1200001','T=10','L=50','tuneK=10']),
                #NmslibMPLSH(['desiredRecall=0.90','H=1200001','T=10','L=50','tuneK=10']),
                #NmslibMPLSH(['desiredRecall=0.85','H=1200001','T=10','L=50','tuneK=10']),
                #NmslibMPLSH(['desiredRecall=0.80','H=1200001','T=10','L=50','tuneK=10']),
                #NmslibMPLSH(['desiredRecall=0.7','H=1200001','T=10','L=50','tuneK=10']),
                #NmslibMPLSH(['desiredRecall=0.6','H=1200001','T=10','L=50','tuneK=10']),
                #NmslibMPLSH(['desiredRecall=0.5','H=1200001','T=10','L=50','tuneK=10']),
                #NmslibMPLSH(['desiredRecall=0.4','H=1200001','T=10','L=50','tuneK=10']),
                #NmslibMPLSH(['desiredRecall=0.3','H=1200001','T=10','L=50','tuneK=10']),
                #NmslibMPLSH(['desiredRecall=0.2','H=1200001','T=10','L=50','tuneK=10']),
                #NmslibMPLSH(['desiredRecall=0.1','H=1200001','T=10','L=50','tuneK=10']),
               #]

    'SW-graph(nmslib)':[
                Nmslib('small_world_rand', ['NN=20', 'initIndexAttempts=4', 'initSearchAttempts=48']),
                Nmslib('small_world_rand', ['NN=20', 'initIndexAttempts=4', 'initSearchAttempts=32']),
                Nmslib('small_world_rand', ['NN=20', 'initIndexAttempts=4', 'initSearchAttempts=16']),
                Nmslib('small_world_rand', ['NN=20', 'initIndexAttempts=4', 'initSearchAttempts=8']),
                Nmslib('small_world_rand', ['NN=20', 'initIndexAttempts=4', 'initSearchAttempts=4']),
                Nmslib('small_world_rand', ['NN=20', 'initIndexAttempts=4', 'initSearchAttempts=2']),

     # Keep it two attempts until recall approaches 80.
                Nmslib('small_world_rand', ['NN=17', 'initIndexAttempts=4', 'initSearchAttempts=2']),
                Nmslib('small_world_rand', ['NN=14', 'initIndexAttempts=4', 'initSearchAttempts=2']),
                Nmslib('small_world_rand', ['NN=11', 'initIndexAttempts=5', 'initSearchAttempts=2']),
                Nmslib('small_world_rand', ['NN=8',  'initIndexAttempts=5', 'initSearchAttempts=2']),
                Nmslib('small_world_rand', ['NN=5',  'initIndexAttempts=5', 'initSearchAttempts=2']),
                Nmslib('small_world_rand', ['NN=3',  'initIndexAttempts=5', 'initSearchAttempts=2']),
               ],

    #'BallTree(nmslib)':[
                #Nmslib('vptree', ['bucketSize=100','alphaLeft=2.8163','alphaRight=1.62105','expLeft=1','expRight=1']),
                #Nmslib('vptree', ['bucketSize=100','alphaLeft=2.17167','alphaRight=2.1022','expLeft=1','expRight=1']),
                #Nmslib('vptree', ['bucketSize=100','alphaLeft=3.42251','alphaRight=2.17167','expLeft=1','expRight=1']),
                #Nmslib('vptree', ['bucketSize=100','alphaLeft=4.58502','alphaRight=2.31747','expLeft=1','expRight=1']),
                #Nmslib('vptree', ['bucketSize=100','alphaLeft=3.31309','alphaRight=3.77291','expLeft=1','expRight=1']),
                #Nmslib('vptree', ['bucketSize=100','alphaLeft=4.73644','alphaRight=4.58502','expLeft=1','expRight=1']),
                #Nmslib('vptree', ['bucketSize=100','alphaLeft=11.3879','alphaRight=3.7729','expLeft=1','expRight=1']),
                #Nmslib('vptree', ['bucketSize=100','alphaLeft=10.6714','alphaRight=6.1424','expLeft=1','expRight=1']),
                #Nmslib('vptree', ['bucketSize=100','alphaLeft=10.6714','alphaRight=12.1525','expLeft=1','expRight=1']),
                #Nmslib('vptree', ['bucketSize=100','alphaLeft=61.954','alphaRight=24.1466','expLeft=1','expRight=1']),
               #],
}

# Delete contents from previous runs!!!
# If you don't delete, this leaves some "outliers" in the plot
#f = open('data.tsv', 'w+')
#f.close()


X_train, X_test = get_dataset(which='glove')

# Prepare queries
bf.fit(X_train)
queries = []
for x in X_test:
    correct = bf.query(x, 10)
    queries.append((x, correct))
    if len(queries) % 100 == 0:
        print len(queries), '...'

algos_already_ran = set()
if os.path.exists('data.tsv'):
    for line in open('data.tsv'):
        algos_already_ran.add(line.strip().split('\t')[1])

algos_flat = []

for library in algos.keys():
    for algo in algos[library]:
        if algo.name not in algos_already_ran:
            algos_flat.append((library, algo))

random.shuffle(algos_flat)

for library, algo in algos_flat:
    print algo.name, '...'
    # Spawn a subprocess to force the memory to be reclaimed at the end
    p = multiprocessing.Process(target=run_algo, args=(library, algo))
    p.start()
    p.join()
