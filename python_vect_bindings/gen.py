import numpy as np

data = np.random.rand(100000,1000)

np.savetxt("/home/bileg/foo.txt", data, delimiter="\t")

