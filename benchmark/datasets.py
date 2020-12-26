import collections
import os

from data_utils import download_and_unpack, \
                read_dense_from_text, read_sparse_from_text, \
                save_dense, save_sparse, \
                NP_SUFF, NPZ_SUFF

BASE_URL='http://boytsov.info/datasets/nmslib_benchmark/'
TEXT_SUFF='.txt'
DATA_PROC_IND_SUFF='.data_proc'

VectorDataProp = collections.namedtuple('VectorDataProp',
                                        'url type')

VECTOR_SPARSE = 'vector_sparse'
VECTOR_DENSE = 'vector_dense'

VECTOR_DATA = {
    'wiki250K' : VectorDataProp(url=f'{BASE_URL}/wikipedia250K.txt.bz2', type=VECTOR_SPARSE),
    'glove100d': VectorDataProp(url=f'{BASE_URL}/glove_noword.6B.100d.txt.bz2', type=VECTOR_DENSE),
    'sif1m' : VectorDataProp(url=f'{BASE_URL}/sift_texmex_base1m.txt.bz2', type=VECTOR_DENSE),
    'final32' : VectorDataProp(url=f'{BASE_URL}/final32.txt.bz2', type=VECTOR_DENSE)
}


def get_bin_suff(data_type):
    if data_type == VECTOR_DENSE:
        return NP_SUFF
    elif data_type == VECTOR_SPARSE:
        return NPZ_SUFF
    else:
        raise Exception(f'Illegal data type: {prop.type}')


def download_and_process_data(dst_dir):
    """This function:
       1. Downloads all data.
       2. Unpacks it.
       3. Converts to numpy/scipy format

    :param dst_dir: destination directory
    """
    for name, prop in VECTOR_DATA.items():
        data_done_path  = os.path.join(dst_dir, name + DATA_PROC_IND_SUFF)
        data_text_path = os.path.join(dst_dir, name + TEXT_SUFF)
        data_bin_path = os.path.join(dst_dir, name)


        if os.path.exists(data_done_path):
            print(f'Ignore already processed dataset {name}')
            continue

        download_and_unpack(url=prop.url, dst_dir=dst_dir, dst_name=data_text_path)

        if prop.type == VECTOR_DENSE:
            print('Converting to numpy format')
            save_dense(data_bin_path, read_dense_from_text(data_text_path))
        elif prop.type == VECTOR_SPARSE:
            print('Converting to scipy format')
            save_sparse(data_bin_path, read_sparse_from_text(data_text_path))
        else:
            raise Exception(f'Illegal data type: {prop.type}')

        # Create an empty file indicating we are done processing
        # Thus we won't redo the processing job again.
        with open(data_done_path, 'w'):
            pass


