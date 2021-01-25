import numpy as np
import collections
import os
import gc
import subprocess
import pandas
import nmslib

from sklearn.neighbors import NearestNeighbors
from scipy.special import rel_entr
from time import time
from datasets import VECTOR_SPARSE, VECTOR_DENSE
from misc_utils import  to_int, del_files_with_prefix, dict_param_to_nsmlib_bin_str

ExperResult = collections.namedtuple('ExperResult', 'recall index_time query_time qps')

DIST_COMPUTE_BATCH_SIZE=100

METHOD_HNSW='hnsw'

PHASE_NEW_INDEX = 'new'
PHASE_RELOAD_INDEX = 'reload'

DIST_L1 = 'l1'
DIST_L2 = 'l2'
DIST_L3 = 'l3'
DIST_LINF = 'linf'

DIST_COSINE = 'cosine'
DIST_INNER_PROD = 'inner_prod'

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
    (VECTOR_DENSE, DIST_L3)         : 'lp:p=3',
    (VECTOR_DENSE, DIST_COSINE)     : 'cosinesimil',
    (VECTOR_SPARSE, DIST_COSINE)    : 'cosinesimil_sparse_fast',
    (VECTOR_DENSE, DIST_INNER_PROD) : 'negdotprod',
    (VECTOR_SPARSE, DIST_INNER_PROD): 'negdotprod_sparse_fast'
}

RECALL_FIELD='Recall'
METHOD_NAME_FIELD='MethodName'
INDEX_PARAMS_FIELD='IndexParams'
QUERY_PARAMS_FIELD='QueryTimeParams'
INDEX_TIME_FIELD='IndexTime'
QPS_FIELD='QueryPerSec'

def get_neighbors_from_dists(dists, K):
    """Compute neighbors based on the array of sorted distances.

    :param dists:  an input distance array of he shape: <#of queries> X <# of data points>
    :param K: a number of neighbors to compute
    :return: an output in the shape <#of queries> X min(K, <# of data points>)
    """
    dists_sorted = np.argsort(dists, axis=-1)
    return dists_sorted[:,:K]


def inner_prod_neighbor_dists(data_matrix, query_matrix_batch):
    """Compute values of the sparse OR dense-vector inner-product.

    :param data_matrix:             data matrix
    :param query_matrix_batch:      query matrix
    :return: an output in the shape <#of queries> X min(K, <# of data points>)
    """

    dists_batch = query_matrix_batch.dot(data_matrix.transpose())
    if type(query_matrix_batch) != np.ndarray:
        # The result of dot + todense operator is going to be numpy.matrix,
        # which is not really a usable data type
        dists_batch = np.array(dists_batch.todense())

    # Take a minus, because larger inner product values denote closer data points
    return -dists_batch


def kldiv_neighbor_dists(data_matrix, query_matrix_batch):
    """Compute values of the KL-divergence for dense vectors.

    :param data_matrix:             data matrix
    :param query_matrix_batch:      query matrix
    :return: an output in the shape <#of queries> X min(K, <# of data points>)
    """

    dists_batch = []
    for k in range(len(query_matrix_batch)):
        v = rel_entr(data_matrix, query_matrix_batch[k])
        dists_batch.append(np.sum(v, axis=-1))

    return dists_batch


def compute_neighbors_batched(data_matrix, query_matrix, K,
                              batch_processor, batch_size):
    """Compute neighbors in a batched fashion where a query is split into batches.

    :param data_matrix:      data matrix
    :param query_matrix:     query matrix
    :param K:                the number of neighbors
    :param batch_processor:  a distance-specific function to compute distances between all data elements and queries 
                             in a given batch
    :param batch_size
    :return: an output in the shape <#of queries> X min(K, <# of data points>)
    """
    query_qty = query_matrix.shape[0]
    neighbors = []

    # If the number of queries is too large, we run out of memory
    # unless we compute neighbors in batches and merge the result
    for i in range(0, query_qty, batch_size):
        gc.collect()
        dists_batch = batch_processor(data_matrix, query_matrix[i : i + batch_size])
        neighbors.append(get_neighbors_from_dists(dists_batch, K))

    return np.concatenate(neighbors, axis=0)


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
        return compute_neighbors_batched(data, queries, K, inner_prod_neighbor_dists, DIST_COMPUTE_BATCH_SIZE)

    elif dist_type == DIST_KL_DIV:
        return compute_neighbors_batched(data, queries, K, kldiv_neighbor_dists, DIST_COMPUTE_BATCH_SIZE)

    else:
        raise Exception(f'Unsupported distance: {dist_type}')


def make_index_file_name(work_dir, method_name, space_name, index_time_params):
    return os.path.join(work_dir,
                 f'index_{method_name}_{space_name}_' + dict_param_to_nsmlib_bin_str(index_time_params))


def make_gold_standard_file_name(work_dir, method_name, space_name, index_time_params):
    return os.path.join(work_dir,
                 f'gs_{method_name}_{space_name}_' + dict_param_to_nsmlib_bin_str(index_time_params))


def make_result_file_name_pref(work_dir, method_name, space_name, index_time_params):
    return os.path.join(work_dir,
                 f'result_{method_name}_{space_name}_' + dict_param_to_nsmlib_bin_str(index_time_params))


def make_log_file_name(work_dir, method_name, space_name, index_time_params):
    return os.path.join(work_dir,
                 f'{method_name}_{space_name}_' + dict_param_to_nsmlib_bin_str(index_time_params) + '.log')


def get_result_file_name(result_file_name_pref, K):
    return f'{result_file_name_pref}_K={K}.dat'


def run_binary_test_case(binary_dir,
                         data_file, query_qty,
                         method_name, space_name,
                         index_file_name, gold_standard_file_name, result_file_name_pref, log_file_name,
                         index_time_params, query_time_param_arr,
                         K, repeat_qty,
                         num_threads, max_data_qty=None):
    """Run a single test of a benchmark of the binary executable (experiment), which uses
       the same set of index-time parameters and possibly multiple set of query-time parameters.
       Each test is repeated repeat_qty: times.

    :param binary_dir             a directory with the binary to test
    :param data_file:             a data file
    :param query_qty:             a number of queries to used (from the data file)
    :param method_name:           NMSLIB method name
    :param space_name:            NMSLIB space type
    :param index_file_name:       the file to store the index. If the index is already present, the binary
                                  will just load this index without trying to create a new one.
    :param gold_standard_file_name:  the file to store the gold standard data.
    :param result_file_name_pref: a prefix for the output files
    :param log_file_name:         a name of the log file
    :param index_time_params:     index-time parameters
    :param query_time_param_arr:  an array of query-time parameters
    :param K:                     the number of neighbors
    :param repeat_qty:            a # of times to repeat queries
    :param num_threads:           # of threads to use (or zero to use all threads)
    :param max_data_qty:          an optional maximum # of data points to use (useful for debugging)

    :return: an array of ExperResults objects
    """
    args = [ os.path.join(binary_dir, 'experiment')]

    del_files_with_prefix(result_file_name_pref)

    index_time_params_str = dict_param_to_nsmlib_bin_str(index_time_params)

    args.extend(['--dataFile', f'{data_file}',
                 '--recallOnly', '1',  # HNSW tests needs this option
                 '--threadTestQty', f'{num_threads}',
                 # we are using only one split and the best time over repeated experiments
                 '--testSetQty', '1',
                 '--maxNumQuery', f'{query_qty}',
                 '-l', f'{log_file_name}',
                 '-s', space_name,
                 '-k', f'{K}',
                 '-o', f'{result_file_name_pref}',
                 '-g', f'{gold_standard_file_name}',
                 '-m', f'{method_name}',
                 '-S', f'{index_file_name}',
                 '-L', f'{index_file_name}',
                 '-c', f'{index_time_params_str}'])
    if max_data_qty is not None:
        args.extend(['--maxNumData', f'{max_data_qty}'])

    for query_time_params in query_time_param_arr:
        args.extend(repeat_qty * ['-t', f'{dict_param_to_nsmlib_bin_str(query_time_params)}'])

    try:
        subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    except subprocess.CalledProcessError as e:
        print(e)
        print(e.stderr.decode())
        raise Exception('Error running experimental binary!')

    res = []
    result_file_name = get_result_file_name(result_file_name_pref, K)
    dt = pandas.read_csv(result_file_name, delimiter='\t', quotechar='"')
    exp_qty = repeat_qty * len(query_time_param_arr)
    if len(dt) != exp_qty:
        raise Exception(f'The result file has unexpected length, expected {exp_qty}, but got {len(dt)}')

    print(f'Index-time parameters: {index_time_params}')

    for ti in range(len(query_time_param_arr)):
        best_tm_recall = 0
        index_tm = 0
        best_tm = float('inf')
        query_time_params_str = dict_param_to_nsmlib_bin_str(query_time_param_arr[ti])

        for k in range(repeat_qty):
            row_id = ti * repeat_qty + k
            assert method_name == dt[METHOD_NAME_FIELD][row_id], \
                f'Unexpected method name {dt[METHOD_NAME_FIELD][row_id]} in row {row_id+2}'
            assert index_time_params_str == dt[INDEX_PARAMS_FIELD][row_id], \
                f'Unexpected index-time parameters {dt[INDEX_PARAMS_FIELD][row_id]} in row {row_id+2}, expecting {index_time_params_str}'
            assert query_time_params_str == dt[QUERY_PARAMS_FIELD][row_id], \
                f'Unexpected query-time parameters {dt[QUERY_PARAMS_FIELD][row_id]} in row {row_id+2}, expecting {query_time_params_str}'

            # NMSLIB produces query time in MILLIseconds, but index time in seconds, it also measures time as latency
            # to be consistent with our Python binding tests, let's take the number of query per second and convert
            # it to time
            # Make explicit casts to types that are always possible to serialize to JSON (numpy.int64 isn't that type!)
            query_tm = float(query_qty / dt[QPS_FIELD][row_id])
            recall = float(dt[RECALL_FIELD][row_id])
            if k == 0:
                index_tm = float(dt[INDEX_TIME_FIELD][row_id])

            if query_tm < best_tm:
                best_tm = query_tm
                best_tm_recall = recall

        qps = query_qty / best_tm
        print('Query time parameters: ', query_time_params, f'{K}-NN recall: recall: {recall:.3f} QPS: {qps:g}')

        res.append(ExperResult(recall=best_tm_recall, index_time=index_tm, query_time=best_tm, qps=qps)._asdict())

    return res


def benchamrk_binary(work_dir, binary_dir,
                       dist_type, data_type,
                       method_name, index_time_params, query_time_param_arr,
                       data_file, query_qty,
                       K, repeat_qty,
                       num_threads, max_data_qty=None):
    """Carry out a benchmark of the binary executable (experiment) in two phases.
       In phase 1 we create the index from scratch. In phase 2 we reload it from disk.
       In each phase, for each set of query parameters we repeat the search procedure repeat_qty: times.

    :param work_dir               working directory to store indices.
    :param binary_dir             a directory with the binary to test
    :param dist_type:             the type of the distance
    :param data_type:             the type of the data
    :param method_name:           the name of the search method
    :param index_time_params:     a dictionary of index-time parameters
    :param query_time_param_arr:  an array
    :param data_file:             an input data file (to be split into queries and indexable documents)
    :param query_qty:             a number of queries
    :param K:                     the number of neighbors
    :param repeat_qty:            a # of times to repeat queries
    :param num_threads:           # of threads to use (or zero to use all threads)
    :param max_data_qty:          an optional maximum # of data points to use (useful for debugging)

    :return: a dictionary of arrays of ExperResults objects: one array is for newly created
             indices and the second array for indices loaded from the disk.

    """

    print('NMSLIB binary test')
    if not data_type in DATATYPE_MAP:
        raise Exception(f'Invalid data type: {data_type}')

    space_key = (data_type, dist_type)
    if not space_key in SPACENAME_MAP:
        raise Exception(f'Unsupported distance/data type combination: {dist_type}/{data_type}')

    space_name = SPACENAME_MAP[space_key]

    res = {
            PHASE_NEW_INDEX : [],
            PHASE_RELOAD_INDEX : []
    }

    log_file_name = make_log_file_name(work_dir=work_dir,
                                        method_name=method_name, space_name=space_name,
                                        index_time_params=index_time_params)

    index_file_name = make_index_file_name(work_dir=work_dir,
                                            method_name=method_name, space_name=space_name,
                                            index_time_params=index_time_params)
    gold_standard_file_name = make_gold_standard_file_name(work_dir=work_dir,
                                            method_name=method_name, space_name=space_name,
                                            index_time_params=index_time_params)
    result_file_name_pref = make_result_file_name_pref(work_dir=work_dir,
                                             method_name=method_name, space_name=space_name,
                                             index_time_params=index_time_params)

    del_files_with_prefix(index_file_name)
    del_files_with_prefix(gold_standard_file_name)

    for phase in [PHASE_NEW_INDEX, PHASE_RELOAD_INDEX]:
        # We call the run_binary_test_case twice in exactly the same way, but we delete the index prior
        # to calling it. The first time run_binary_test_case is called, it will create an index and save it,
        # but the second time it will simply read the existing index from disk.
        # Likewise, we are going to compute gold standard data during the first call and re-using
        # it in a second one.
        res[phase] = run_binary_test_case(binary_dir=binary_dir,
                         data_file=data_file, query_qty=query_qty,
                         method_name=method_name, space_name=space_name,
                         index_file_name=index_file_name,
                         gold_standard_file_name=gold_standard_file_name,
                         result_file_name_pref=result_file_name_pref,
                         log_file_name=log_file_name,
                         index_time_params=index_time_params, query_time_param_arr=query_time_param_arr,
                         K=K, repeat_qty=repeat_qty,
                         num_threads=num_threads, max_data_qty=max_data_qty)

    print('---------------------')

    return res


def benchmark_bindings(work_dir,
                       dist_type, data_type,
                       method_name, index_time_params, query_time_param_arr,
                       data, queries, K, repeat_qty,
                       num_threads, max_data_qty=None):
    """Carry out a benchmark of Python bindings in two phases.
       In phase 1 we create the index from scratch. In phase 2 we reload it from disk.
       In each phase, for each set of query parameters we repeat the search procedure repeat_qty: times.

    :param work_dir               working directory to store indices.
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
    :param max_data_qty:          an optional maximum # of data points to use (useful for debugging)

    :return: a dictionary of arrays of ExperResults objects: one array is for newly created
             indices and the second array for indices loaded from the disk.

    """
    import nmslib

    if not data_type in DATATYPE_MAP:
        raise Exception(f'Invalid data type: {data_type}')

    space_key = (data_type, dist_type)
    if not space_key in SPACENAME_MAP:
        raise Exception(f'Unsupported distance/data type combination: {dist_type}/{data_type}')

    space_name = SPACENAME_MAP[space_key]

    if max_data_qty is not None:
        data=data[0:max_data_qty]

    query_qty = queries.shape[0]
    data_qty = data.shape[0]


    print('Python binding tests')
    print(f'Computing gold standard data, distance: {dist_type} K={K}, data type: {data_type} '
          f'# of data points: {data_qty} # of queries: {query_qty}')

    gold_stand = compute_neighbors(dist_type, data, queries, K)

    res = {
            PHASE_NEW_INDEX : [],
            PHASE_RELOAD_INDEX : []
    }

    index_file_name = make_index_file_name(work_dir=work_dir,
                                            method_name=method_name, space_name=space_name,
                                            index_time_params=index_time_params)

    del_files_with_prefix(index_file_name)

    # TODO parsing parameters is broken and needs to be fixed
    # # NMSLIB space name can have parameters, which need to be extracted
    # tmp = space_name.split(':')
    # space_name = tmp[0]
    # assert len(tmp) < 3
    # space_params = None
    # if len(tmp) == 2:
    #     space_params = tmp[1].split(',')
    #
    # print(f'Space name: {space_name}  params: {space_params}')

    for phase in [PHASE_NEW_INDEX, PHASE_RELOAD_INDEX]:
        index = nmslib.init(space=space_name,
                            method=method_name,
                            data_type=DATATYPE_MAP[data_type])

        if phase == PHASE_NEW_INDEX:
            print(f'Indexing data')
            index.addDataPointBatch(data)

            start = time()

            index.createIndex(index_time_params)

            end = time()
            index_tm = end - start
            print(f'Indexing took: {index_tm:.1f}')

            index.saveIndex(index_file_name, save_data=True)
        else:
            assert phase == PHASE_RELOAD_INDEX
            index_tm = 0 # no index

            print(f'Loading index & data')

            index.loadIndex(index_file_name, load_data=True)

        print(f'Index-time parameters: {index_time_params}')

        for query_time_params in query_time_param_arr:
            index.setQueryTimeParams(query_time_params)

            best_tm = float('inf')
            best_tm_recall = 0

            for exp_id in range(repeat_qty):
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

                #print(')
                if query_tm < best_tm:
                    best_tm = query_tm
                    best_tm_recall = recall

            qps = query_qty / best_tm
            print('Query time parameters: ', query_time_params, f'{K}-NN recall: recall: {recall:.3f} QPS: {qps:g}')

            res[phase].append(ExperResult(recall=best_tm_recall, index_time=index_tm, query_time=best_tm, qps=qps)._asdict())

    print('---------------------')

    return res
