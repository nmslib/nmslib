import os

def to_int(n):
    """Converts a string to an integer value. Returns None, if the string cannot be converted."""
    try:
        return int(n)
    except ValueError:
        return None


def del_files_with_prefix(file_prefix):
    """Given a prefix, delete all files with this prefix.

    :param   file_prefix:  a file prefix
    """
    dir_name = os.path.dirname(file_prefix)
    if dir_name == '':
        dir_name = '.'
    file_pref_name = os.path.basename(file_prefix)

    for fname in os.listdir(dir_name):
        if fname.startswith(file_pref_name):
            os.unlink(os.path.join(dir_name, fname))


def dict_param_to_nsmlib_bin_str(params):
    """A simple function to convert a dictionary of params to a string that can be
       understood by NMSLIB binary.

    :param params:  a dictionary of parameters, e.g., {'M': 30, 'indexThreadQty': 4}
    :return: a string, e.g., M=30,indexThreadQty=4
    """
    return ','.join([str(k) + '=' + str(v) for k, v in params.items()])
