import gzip
import bz2
import os
import numpy as np
from scipy.sparse import csr_matrix
from scipy.sparse import save_npz, load_npz
from sklearn.model_selection import train_test_split

from urllib.request import urlretrieve

NP_SUFF='.npy'
NPZ_SUFF='.npz'


def split_data(all_data_matrix, test_size=0.1, seed=0):
    """Split data.

    :param all_data_matrix:
    :param test_size:    the test size
    :param seed:         a random seed
    :return:             a tuple: data matrix, query matrix
    """
    (data_matrix, query_matrix) = train_test_split(all_data_matrix, test_size=test_size, random_state=seed)

    return data_matrix, query_matrix


def read_dense_from_text(file_name, max_qty=None):
    """Read dense vectors from a text file.

    :param file_name:    input file name
    :param max_qty:      a maximum # of rows to read
    :return:             a numpy array
    """
    return np.loadtxt(file_name, max_rows=max_qty)


def save_dense(file_name_pref, numpy_matr):
    """A wraper for saving dense vectors.

    :param file_name_pref:   output file name prefix (without npy)
    :param numpy_matr:       dense numpy array
    """
    np.save(file_name_pref, numpy_matr)


def load_dense(file_name_pref):
    """A wrapper for loading dense vectors.

    :param file_name_pref:   input file name prefix (without npy)
    :return: dense numpy array
    """
    return np.load(file_name_pref + NP_SUFF)


def read_sparse_from_text(file_name, max_qty=None):
    """Read dense vectors from a text file.

    :param file_name:    input file name
    :param test_size:    the test size
    :param seed:         a random seed
    :param max_qty:      a maximum # of rows to read
    :return a sparse matrix
    """

    row = []
    col = []
    data = []
    read_qty = 0
    with open(file_name, 'r') as f:
        read_num_ft = 0
        for line in f:
            x = line.replace(':', ' ').strip().split()

            if len(x) % 2 != 0:
                raise (Exception('Poorly formated line %d in file %s' % (read_qty + 1, file_name)))

            # Ignore empty lines
            if len(x) == 0:
                continue

            for i in range(0, len(x), 2):
                row.append(read_qty)
                feat_id = int(x[i])
                read_num_ft = max(read_num_ft, feat_id + 1)
                col.append(feat_id)
                data.append(float(x[i + 1]))

            read_qty = read_qty + 1

            if max_qty is not None and read_qty >= max_qty: break

    all_data_matrix = csr_matrix((np.array(data), (np.array(row), np.array(col))), shape=(read_qty, read_num_ft))

    return all_data_matrix


def save_sparse(file_name_pref, sparse_matr):
    """A wraper for saving sparse vectors/matrices.

    :param file_name_pref:   output file name prefix (without npz)
    :param sparse_matr:      a sparse matrix
    """
    save_npz(file_name_pref, sparse_matr)


def load_sparse(file_name_pref):
    """A wrapper for loading dense vectors.

    :param file_name_pref:   input file name prefix (without npz)
    :return:  a sparse matrix
    """
    return load_npz(file_name_pref + NPZ_SUFF)


class FileWrapper:
    # From https://github.com/oaqa/FlexNeuART/blob/master/scripts/data_convert/convert_common.py
    def __enter__(self):
        return self

    def __init__(self, fileName, flags='r'):
        """Constructor, which opens a regular or gzipped-file

          :param  fileName a name of the file, it has a '.gz' or '.bz2' extension, we open a compressed stream.
          :param  flags    open flags such as 'r' or 'w'
        """
        os.makedirs(os.path.dirname(fileName), exist_ok=True)
        if fileName.endswith('.gz'):
            self._file = gzip.open(fileName, flags)
            self._isCompr = True
        elif fileName.endswith('.bz2'):
            self._file = bz2.open(fileName, flags)
            self._isCompr = True
        else:
            self._file = open(fileName, flags)
            self._isCompr = False

    def write(self, s):
        if self._isCompr:
            self._file.write(s.encode())
        else:
            self._file.write(s)

    def read(self, s):
        if self._isCompr:
            return self._file.read().decode()
        else:
            return self._file.read()

    def close(self):
        self._file.close()

    def __exit__(self, type, value, tb):
        self._file.close()

    def __iter__(self):
        for line in self._file:
            yield line.decode() if self._isCompr else line


def download_and_unpack(url, dst_dir, dst_name):
    """Download and unpack the file.

    :param url:     download URL
    :param dst_dir: destination directory
    :param dst_name: destination file name within the destination directory:
                     it must be different from the name of the downloadable file.
                     For example, if you want to download http://server.com/filename.gz2,
                     the destination file name can be filename, but not filename.gz2.

    :return: unpacked file name
    """
    # It is better to remove the target file to make sure
    # we are not reusing some partially downloaded one
    dst_path = os.path.join(dst_dir, dst_name)
    if os.path.exists(dst_path):
        os.unlink(dst_path)

    basename = os.path.basename(url)
    download_file = os.path.join(dst_dir, basename)
    assert download_file != dst_path, \
        "The name of the downloaded file should be different from the target file name!"
    print(f'Downloading {url} -> {download_file}')

    try:
        urlretrieve(url, download_file)
    except Exception as e:
        print(e)
        raise Exception(f'Error downloading: {url}')


    out = open(dst_path, 'w')

    print(f'Unpacking {url}')
    for line in FileWrapper(download_file):
        out.write(line)

    os.unlink(download_file)
