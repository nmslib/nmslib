import numpy as np
import nmslib
import collections

from sklearn.neighbors import NearestNeighbors
from scipy.special import rel_entr
from time import time
from datasets import VECTOR_SPARSE, VECTOR_DENSE

ExperResult = collections.namedtuple('ExperResult',
                                     'recall index_time query_time qps')

DIST_L3 = 'l3'
DIST_L2 = 'l2'
DIST_COSINE = 'cosine'
DIST_INNER_PROD = 'inner_prod'
DIST_L1 = 'l1'
DIST_LINF = 'linf'
DIST_KL_DIV = 'kldiv'

SKLEARN_MAP = {
    DIST_L2 : 'l2',
    DIST_COSINE : 'cosine',
    DIST_L1 : 'l1',
    DIST_LINF : 'chebyshev'
}

DATATYPE_MAP = {
    VECTOR_DENSE: nmslib.DataType.DENSE_VECTOR,
    VECTOR_SPARSE: nmslib.DataType.SPARSE_VECTOR
}

# A space name is a combination of the distance type and the data type
SPACENAME_MAP = {
    (VECTOR_DENSE, DIST_KL_DIV)     : 'kldivfast',
    (VECTOR_DENSE, DIST_LINF)       : 'linf',
    (VECTOR_DENSE, DIST_L1)         : 'l1',
    (VECTOR_DENSE, DIST_L2)         : 'l2',
    (VECTOR_DENSE, DIST_L3)         : 'l3',
    (VECTOR_DENSE, DIST_COSINE)     : 'cosinesimil',
    (VECTOR_SPARSE, DIST_COSINE)    : 'cosinesimil_sparse_fast',
    (VECTOR_DENSE, DIST_INNER_PROD) : 'negdotprod',
    (VECTOR_SPARSE, DIST_INNER_PROD): 'negdotprod_sparse_fast'
}

def to_int(n):
    """Converts a string to an integer value. Returns None, if the string cannot be converted."""
    try:
        return int(n)
    except ValueError:
        return None


def get_neighbors_from_dists(dists, K):
    """Compute neighbors based on the array of sorted distances.

    :param dists:  an input distance array of he shape: <#of queries> X <# of data points>
    :param K: a number of neighbors to compute
    :return: an output in the shape <#of queries> X min(K, <# of data points>)
    """
    dists_sorted = np.argsort(dists, axis=-1)
    return dists_sorted[:,:K]


def compute_kl_div_neighbors(data_matrix, query_matrix, K):
    """Compute neighbors for the KL-divergence. By default,
       in NMSLIB, queries are left, i.e., the data object is
       the first (left) argument.

    :param data_matrix:      data matrix
    :param query_matrix:     query matrix
    :param K:                the number of neighbors
    :return: an output in the shape <#of queries> X min(K, <# of data points>)
    """
    dists = []
    for i in range(len(query_matrix)):
        v = rel_entr(data_matrix, query_matrix[i])
        dists.append(np.sum(v, axis=-1))

    return get_neighbors_from_dists(np.stack(dists, axis=0), K)


def compute_neighbors(dist_type, data, queries, K):
    """Compute neighbors for a given distance.

    :param dist_type:        the type of the distance
    :param data:             data
    :param queries:          queries
    :param K:                the number of neighbors
    :return: an output in the shape <#of queries> X min(K, <# of data points>)
    """

    dist_type = dist_type.lower()

    if dist_type in SKLEARN_MAP:
        sindx = NearestNeighbors(n_neighbors=K, metric=SKLEARN_MAP[dist_type], algorithm='brute', n_jobs=-1)
        sindx.fit(data)

        return sindx.kneighbors(queries, return_distance=False)

    elif dist_type.startswith('l') and len(dist_type) > 1 and to_int(dist_type[1:]) > 0:
        sindx = NearestNeighbors(n_neighbors=K, metric='minkowski', p=to_int(dist_type[1:]), algorithm='brute', n_jobs=-1)
        sindx.fit(data)
        return sindx.kneighbors(queries, return_distance=False)

    elif dist_type == DIST_INNER_PROD:
        dists = queries.dot(data.transpose())
        if type(queries) != np.ndarray:
            # The result of dot + todense operator is going to be numpy.matrix,
            # which is not really a usable data type
            dists = np.array(dists.todense())

        # Take a minus, because larger inner product values denote closer data points
        return get_neighbors_from_dists(-dists, K)

    elif dist_type == DIST_KL_DIV:
        return compute_kl_div_neighbors(data, queries, K)

    else:
        raise Exception(f'Unsupported distance: {dist_type}')


def benchmark_bindings(dist_type, data_type,
                       method_name, index_time_params, query_time_param_arr,
                       data, queries, K, repeat_qty,
                       num_threads=0):
    """Carry out a benchmark of Python bindings:
       for each set of query parameters carry out search repeat_qty: times.

    :param dist_type:             the type of the distance
    :param data_type:             the type of the data
    :param method_name:           the name of the search method
    :param index_time_params:     a dictionary of index-time parameters
    :param query_time_param_arr:  an array
    :param data:                  data
    :param queries:               queries
    :param K:                     the number of neighbors
    :param repeat_qty:            a # of times to repeat queries
    :param num_threads:           # of threads to use (or zero to use all threads)

    :return: an array of ExperResults objects

    """
    if not data_type in DATATYPE_MAP:
        raise Exception(f'Invalid data type: {data_type}')

    space_key = (data_type, dist_type)
    if not space_key in SPACENAME_MAP:
        raise Exception(f'Unsupported distance/data type combination: {dist_type}/{data_type}')

    space_name = SPACENAME_MAP[space_key]

    query_qty = queries.shape[0]
    data_qty = data.shape[0]

    print(f'Computing gold standard data, distance: {dist_type} K={K}, data type: {data_type} '
          f'# of data points: {data_qty} # of queries: {query_qty}')

    gold_stand = compute_neighbors(dist_type, data, queries, K)

    res = []

    print(f'Indexing data')
    start = time()

    index = nmslib.init(method=method_name, space=space_name, data_type=DATATYPE_MAP[data_type])
    index.addDataPointBatch(data)
    index.createIndex(index_time_params)

    end = time()
    index_tm = end - start
    print(f'Indexing took: {index_tm:.1f}')

    for query_time_params in query_time_param_arr:
        index.setQueryTimeParams(query_time_params)
        print('Query time parameters: ', query_time_params)
        best_tm = float('inf')
        best_tm_recall = 0


        for exp_id in range(repeat_qty):
            print(f'Running experiment # {exp_id+1}')
            start = time()
            nbrs = index.knnQueryBatch(queries, k=K, num_threads=num_threads)
            end = time()

            query_tm = end - start

            recall=0
            for i in range(query_qty):
                correct_set = set(gold_stand[i])
                ret_set = set(nbrs[i][0])
                recall = recall + float(len(correct_set.intersection(ret_set))) / len(correct_set)
            recall = recall / query_qty
            qps = query_qty / query_tm
            print(f'{K}-NN recall: recall: {recall:.3f} QPS: {qps:g}')
            if query_tm < best_tm:
                best_tm = query_tm
                best_tm_recall = recall

        res.append(ExperResult(recall=best_tm_recall, index_time=index_tm, query_time=best_tm, qps= query_qty / best_tm))


    return res
