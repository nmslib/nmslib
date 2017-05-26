import time

class TimeIt(object):
    def __init__(self, name):
        self.name = name

    def __enter__(self):
        self.t = time.time()
        return None

    def __exit__(self, type, value, traceback):
        print('%s took %.2f seconds' % (self.name, time.time() - self.t))

