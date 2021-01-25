#!/usr/bin/env python
import sys
import argparse
import nmslib
import json

DEFAULT_HNSW_INDEX_TIME_PARAM = {'M': 20, 'efConstruction': 200, 'post': 0}

DEFAULT_QUERY_QTY=5000

from eval import *
from datasets import *
from data_utils import *

sys.path.append('.')

TestCase = collections.namedtuple('TestCase',
                                  'dataset_name dist_type K method_name index_time_params query_time_param_arr')

ExpandExperResult = collections.namedtuple('ExpandExperResult',
                                           'case_id nmslib_version '
                                           'dataset_name dist_type K '
                                           'is_binary is_index_reload '
                                           'repeat_qty query_qty max_data_qty '
                                           'num_threads '
                                           'result_list')

DEFAULT_HSNW_QUERY_TIME_PARAM_ARR = [{'ef':25}, {'ef': 50}, {'ef':100}, {'ef':250}, {'ef':500}, {'ef':1000}, {'ef': 2000 }]

TEST_CASES = [
    # DENSE DATA
    # L1 SIFT
    TestCase(dataset_name=SIFT1M, dist_type=DIST_L1, K=10, method_name=METHOD_HNSW,
             index_time_params=DEFAULT_HNSW_INDEX_TIME_PARAM, query_time_param_arr=DEFAULT_HSNW_QUERY_TIME_PARAM_ARR),
    # L2 SIFT
    TestCase(dataset_name=SIFT1M, dist_type=DIST_L2, K=10, method_name=METHOD_HNSW,
             index_time_params=DEFAULT_HNSW_INDEX_TIME_PARAM, query_time_param_arr=DEFAULT_HSNW_QUERY_TIME_PARAM_ARR),
    TestCase(dataset_name=SIFT1M, dist_type=DIST_L2, K=10, method_name=METHOD_HNSW,
             index_time_params=DEFAULT_HNSW_INDEX_TIME_PARAM, query_time_param_arr=DEFAULT_HSNW_QUERY_TIME_PARAM_ARR),

    # TODO
    # L3 requires passing space parameters, but this unfortunately is broken currently
    # L3 SIFT
    # TestCase(dataset_name=SIFT1M, dist_type=DIST_L3, K=10, method_name=METHOD_HNSW,
    #          index_time_params=DEFAULT_HNSW_INDEX_TIME_PARAM, query_time_param_arr=DEFAULT_HSNW_QUERY_TIME_PARAM_ARR),
    # TestCase(dataset_name=SIFT1M, dist_type=DIST_L3, K=10, method_name=METHOD_HNSW,
    #          index_time_params=DEFAULT_HNSW_INDEX_TIME_PARAM, query_time_param_arr=DEFAULT_HSNW_QUERY_TIME_PARAM_ARR),
    # LINF SIFT
    TestCase(dataset_name=SIFT1M, dist_type=DIST_LINF, K=10, method_name=METHOD_HNSW,
             index_time_params=DEFAULT_HNSW_INDEX_TIME_PARAM, query_time_param_arr=DEFAULT_HSNW_QUERY_TIME_PARAM_ARR),
    # cosine GLOVE
    TestCase(dataset_name=GLOVE100D, dist_type=DIST_COSINE, K=10, method_name=METHOD_HNSW,
             index_time_params=DEFAULT_HNSW_INDEX_TIME_PARAM, query_time_param_arr=DEFAULT_HSNW_QUERY_TIME_PARAM_ARR),
    # inner product GLOVE
    TestCase(dataset_name=GLOVE100D, dist_type=DIST_INNER_PROD, K=10, method_name=METHOD_HNSW,
             index_time_params=DEFAULT_HNSW_INDEX_TIME_PARAM, query_time_param_arr=DEFAULT_HSNW_QUERY_TIME_PARAM_ARR),
    # KL-div FINAL32
    TestCase(dataset_name=FINAL32, dist_type=DIST_KL_DIV, K=10, method_name=METHOD_HNSW,
             index_time_params=DEFAULT_HNSW_INDEX_TIME_PARAM, query_time_param_arr=DEFAULT_HSNW_QUERY_TIME_PARAM_ARR),

    # SPARSE DATA
    # cosine Wikipedia
    TestCase(dataset_name=WIKI250K, dist_type=DIST_COSINE, K=10, method_name=METHOD_HNSW,
             index_time_params=DEFAULT_HNSW_INDEX_TIME_PARAM, query_time_param_arr=DEFAULT_HSNW_QUERY_TIME_PARAM_ARR),
    # inner product Wikipedia
    TestCase(dataset_name=WIKI250K, dist_type=DIST_INNER_PROD, K=10, method_name=METHOD_HNSW,
             index_time_params=DEFAULT_HNSW_INDEX_TIME_PARAM, query_time_param_arr=DEFAULT_HSNW_QUERY_TIME_PARAM_ARR),
]


def main():
    parser = argparse.ArgumentParser('NMSLIB benchmarks')

    parser.add_argument('--dataset_dir',
                        metavar='dataset dir',
                        help='a directory to store/retrieve datasets',
                        type=str,
                        required=True)
    parser.add_argument('--binding_ver',
                        metavar='binding ver. to test',
                        help='Specify this variable to test a specific version of bindings. '
                             'Testing will be aborted if the version does not match',
                        type=str,
                        default=None)
    parser.add_argument('--binary_dir',
                        metavar='NSMLIB binary dir',
                        help='a directory with compiled NMSLIB binaries',
                        type=str,
                        default=None)
    parser.add_argument('--max_data_qty',
                        metavar='max # of points',
                        help='a max # of data points to use in a test',
                        type=int, default=None)
    parser.add_argument('--repeat_qty',
                        metavar='# of reps',
                        help='# of times to repeat each test',
                        type=int, default=3)
    parser.add_argument('--query_qty',
                        metavar='# of queries',
                        help='# queries',
                        type=int,
                        default=DEFAULT_QUERY_QTY)
    parser.add_argument('--num_threads',
                        metavar='# of query threads',
                        help='# of threads to use for querying (only)',
                        type=int, default=4)
    parser.add_argument('--work_dir',
                        metavar='working dir',
                        type=str,
                        help='a directory to store indices, gold standard files, etc.',
                        required=True)
    parser.add_argument('--output',
                        metavar='output file',
                        type=str,
                        help='a file to store benchmark results',
                        required=True)
    parser.add_argument('--dist_type',
                        metavar='distance type',
                        type=str,
                        help='an optional distance type (if specified we run tests only for this distance)',
                        default=None,
                        choices=[DIST_INNER_PROD, DIST_KL_DIV, DIST_COSINE, DIST_L1, DIST_L2, DIST_LINF])
    parser.add_argument('--data_type',
                        metavar='data type',
                        type=str,
                        help='an optional type of the data (if specified we run tests only for this data type)',
                        default=None,
                        choices=[VECTOR_DENSE, VECTOR_SPARSE])
    parser.add_argument('--dataset_name',
                        type=str,
                        metavar='dataset name',
                        help='an optional dataset type (if specified we run tests only for this dataset)',
                        default=None)

    print(f'Installed version of NMSLIB bindings: {nmslib.__version__}')

    args = parser.parse_args()

    if args.binary_dir is None and args.binding_ver is None:
        print('You need to specify either --binary_dir or --binding_ver')
        sys.exit(1)

    # First make sure we have all the data
    download_and_process_data(args.dataset_dir)

    results = []

    if args.binding_ver is not None:
        # Check installed bindings
        ver = nmslib.__version__
        if ver != args.binding_ver:
            print('A mismatch between installed bindings version {ver} and requested one to test: {args.binding_ver}')
            sys.exit(1)

    for case_id, case in enumerate(TEST_CASES):
        if args.dataset_name is not None and args.dataset_name != case.dataset_name:
            print(f'Ignoring dataset {case.dataset_name}')
            continue
        if args.dist_type is not None and args.dist_type != case.dist_type:
            print(f'Ignoring distance {case.dist_type}')
            continue

        data_type = DATASET_DESC[case.dataset_name].type

        if args.data_type is not None and args.data_type != data_type:
            print(f'Ignoring data type {data_type}')
            continue

        if args.binding_ver is not None: # Python bindings test
            if data_type == VECTOR_DENSE:
                all_data = load_dense(os.path.join(args.dataset_dir, case.dataset_name ))
            elif data_type == VECTOR_SPARSE:
                all_data = load_sparse(os.path.join(args.dataset_dir, case.dataset_name ))
            else:
                raise Exception(f'Illegal data type: {prop.type}')

            data, queries = split_data(all_data, test_size=args.query_qty)

            result_dict = benchmark_bindings(work_dir=args.work_dir,
                                             dist_type=case.dist_type,
                                             data_type=data_type,
                                             method_name=case.method_name,
                                             index_time_params=case.index_time_params,
                                             query_time_param_arr=case.query_time_param_arr,
                                             data=data, queries=queries,
                                             K=case.K, repeat_qty=args.repeat_qty,
                                             num_threads=args.num_threads, max_data_qty=args.max_data_qty)

            for phase in [PHASE_NEW_INDEX, PHASE_RELOAD_INDEX]:
                results.append(ExpandExperResult(case_id=case_id,
                                                 nmslib_version=nmslib.__version__,
                                                 dataset_name=case.dataset_name, K=case.K,
                                                 num_threads=args.num_threads, dist_type=case.dist_type,
                                                 is_binary=False, is_index_reload=phase==PHASE_RELOAD_INDEX,
                                                 repeat_qty=args.repeat_qty, query_qty=args.query_qty,
                                                 max_data_qty=args.max_data_qty,
                                                 result_list=result_dict[phase])._asdict())

        if args.binary_dir is not None:
            result_dict = benchamrk_binary(work_dir=args.work_dir,
                                           binary_dir = args.binary_dir,
                                           dist_type=case.dist_type,
                                           data_type=data_type,
                                           method_name=case.method_name,
                                           index_time_params=case.index_time_params,
                                           query_time_param_arr=case.query_time_param_arr,
                                           data_file = os.path.join(args.dataset_dir, case.dataset_name + TEXT_SUFF),
                                           query_qty=args.query_qty,
                                           K=case.K, repeat_qty=args.repeat_qty,
                                           num_threads=args.num_threads, max_data_qty=args.max_data_qty)

            for phase in [PHASE_NEW_INDEX, PHASE_RELOAD_INDEX]:
                results.append(ExpandExperResult(case_id=case_id,
                                                 nmslib_version=None,
                                                 dataset_name=case.dataset_name, K=case.K,
                                                 num_threads=args.num_threads, dist_type=case.dist_type,
                                                 is_binary=True, is_index_reload=phase==PHASE_RELOAD_INDEX,
                                                 repeat_qty=args.repeat_qty, query_qty=args.query_qty,
                                                 max_data_qty=args.max_data_qty,
                                                 result_list=result_dict[phase])._asdict())

    with open(args.output, 'w') as f:
        json.dump(results,
                  f,
                  indent=4) # Indent for a pretty print


if __name__ == "__main__":
    main()
