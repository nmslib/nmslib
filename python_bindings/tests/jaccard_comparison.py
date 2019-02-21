import sys
import numpy as np
import nmslib
import psutil
import logging
import multiprocessing
import time
import os
import threading

MB = 1024 * 1024
CHUNK_SIZE = 10000


class StoppableThread(threading.Thread):
    """Thread class with a stop() method. The thread itself has to check
    regularly for the stopped() condition."""

    def __init__(self, *args, **kwargs):
        super().__init__()
        self._stop_event = threading.Event()

    def stop(self):
        self._stop_event.set()

    def stopped(self):
        return self._stop_event.is_set()


class Timer:
    """ Context manager for timing named blocks of code """
    def __init__(self, name, logger=None):
        self.name = name
        self.logger = logger if logger else logging.getLogger()

    def __enter__(self):
        self.start = time.time()
        self.logger.debug("Starting {}".format(self.name))

    def __exit__(self, type, value, trace):
        self.logger.info("{}: {:0.2f}s".format(self.name, time.time() - self.start))


class PeakMemoryUsage:
    class Worker(StoppableThread):
        def __init__(self, interval, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self.interval = interval
            self.max_rss = self.max_vms = 0

        def run(self):
            process = psutil.Process()
            while not self.stopped():
                mem = process.memory_info()
                self.max_rss = max(self.max_rss, mem.rss)
                self.max_vms = max(self.max_vms, mem.vms)
                time.sleep(self.interval)

    """ Context manager to calculate peak memory usage in a statement block """
    def __init__(self, name, logger=None, interval=1):
        self.name = name
        self.logger = logger if logger else logging.getLogger()
        self.interval = interval
        self.start = time.time()
        self.worker = None

    def __enter__(self):
        if self.interval > 0:
            pid = os.getpid()
            mem = psutil.Process(pid).memory_info()
            self.start_rss, self.start_vms = mem.rss, mem.vms

            self.worker = PeakMemoryUsage.Worker(self.interval)
            self.worker.start()
        return self

    def __exit__(self, _, value, trace):
        if self.worker:
            self.worker.stop()
            self.worker.join()
            self.logger.warning("Peak memory usage for '{}' in MBs: orig=(rss={:0.1f} vms={:0.1f}) "
                              "peak=(rss={:0.1f} vms={:0.1f}) in {:0.2f}s"
                              .format(self.name, self.start_rss / MB, self.start_vms / MB,
                                      self.worker.max_rss / MB,
                                      self.worker.max_vms / MB, time.time() - self.start))


class PsUtil(object):
    def __init__(self, attr=('virtual_memory',), proc_attr=None,
                 logger=None, interval=60):
        """ attr can be multiple methods of psutil (e.g. attr=['virtual_memory', 'cpu_times_percent']) """
        self.ps_mon = None
        self.attr = attr
        self.proc_attr = proc_attr
        self.logger = logger if logger else logging.getLogger()
        self.interval = interval

    def psutil_worker(self, pid):
        root_proc = psutil.Process(pid)
        while True:
            for attr in self.attr:
                self.logger.warning("PSUTIL {}".format(getattr(psutil, attr)()))
            if self.proc_attr:
                procs = set(root_proc.children(recursive=True))
                procs.add(root_proc)
                procs = sorted(procs, key=lambda p: p.pid)

                for proc in procs:
                    self.logger.warning("PSUTIL process={}: {}"
                                      .format(proc.pid, proc.as_dict(self.proc_attr)))

            time.sleep(self.interval)

    def __enter__(self):
        if self.interval > 0:
            self.ps_mon = multiprocessing.Process(target=self.psutil_worker, args=(os.getpid(),))
            self.ps_mon.start()
            time.sleep(1)  # sleep so the first iteration doesn't include statements in the PsUtil context
        return self

    def __exit__(self, type, value, trace):
        if self.ps_mon is not None:
            self.ps_mon.terminate()


def bit_vector_to_str(bit_vect):
    return " ".join(["1" if e else "0" for e in bit_vect])


def bit_vector_sparse_str(bit_vect):
    return " ".join([str(k) for k, b in enumerate(bit_vect) if b])


def run(space, num_elems, nbits):
    np.random.seed(23)
    if space == "bit_jaccard":
        bit_vector_str_func = bit_vector_to_str
        index = nmslib.init(method='hnsw', space=space, data_type=nmslib.DataType.OBJECT_AS_STRING,
                            dtype=nmslib.DistType.FLOAT)
    else:
        bit_vector_str_func =  bit_vector_sparse_str
        index = nmslib.init(method='hnsw', space=space, data_type=nmslib.DataType.OBJECT_AS_STRING,
                            dtype=nmslib.DistType.FLOAT)

    with PeakMemoryUsage(f"All: space={space} nbits={nbits} elems={num_elems}"):
        for i in range(0, num_elems, CHUNK_SIZE):
            strs = []
            for j in range(CHUNK_SIZE):
                a = np.random.rand(nbits) > 0.5
                strs.append(bit_vector_str_func(a))
            index.addDataPointBatch(ids=np.arange(i, i + CHUNK_SIZE), data=strs)

        index.createIndex()

    a = np.ones(nbits)
    ids, distances = index.knnQuery(bit_vector_str_func(a), k=10)
    print(distances)


if __name__ == "__main__":
    np.set_printoptions(linewidth=500)

    logging.basicConfig(level=logging.WARNING)
    space = sys.argv[1]
    num_elems = int(sys.argv[2])
    nbits = int(sys.argv[3])
    run(space, num_elems, nbits)
