#!/usr/bin/env python3
import logging
# Uncomment to print logs to the screen
#logging.basicConfig(level=logging.INFO)

import numpy as np
import sys 
import nmslib 
import time 
import math 

from sklearn.neighbors import NearestNeighbors
from sklearn.datasets.samples_generator import make_blobs

def testHnswRecallL2(dataMatrix, queryMatrix, k, M=30, efC=200, efS=1000, numThreads=4):
  queryQty = queryMatrix.shape[0]
  indexTimeParams = {'M': M, 'indexThreadQty': numThreads, 'efConstruction': efC, 'post' : 0}

  #Indexing
  print('Index-time parameters', indexTimeParams)
  spaceName='l2'
  index = nmslib.init(method='hnsw', space=spaceName, data_type=nmslib.DataType.DENSE_VECTOR) 
  index.addDataPointBatch(dataMatrix)  

  start = time.time()
  index.createIndex(indexTimeParams) 
  end = time.time() 
  print('Indexing time = %f' % (end-start))

  # Setting query-time parameters
  queryTimeParams = {'efSearch': efS}
  print('Setting query-time parameters', queryTimeParams)
  index.setQueryTimeParams(queryTimeParams)

  # Querying
  start = time.time()
  nmslibFound = index.knnQueryBatch(queryMatrix, k=k, num_threads=numThreads)
  end = time.time()
  print('kNN time total=%f (sec), per query=%f (sec), per query adjusted for thread number=%f (sec)' %
        (end - start, float(end - start) / queryQty, numThreads * float(end - start) / queryQty))


  # Computing gold-standard data 
  print('Computing gold-standard data')

  start = time.time()
  sindx = NearestNeighbors(n_neighbors=k, metric='l2', algorithm='brute').fit(dataMatrix)
  end = time.time()

  print('Brute-force preparation time %f' % (end - start))

  start = time.time() 
  bruteForceFound = sindx.kneighbors(queryMatrix)
  end = time.time()

  print('brute-force kNN time total=%f (sec), per query=%f (sec)' % 
       (end-start, float(end-start)/queryQty) )

  # Finally computing recall for every i-th neighbor
  for n in range(k):
    recall=0.0
    for i in range(0, queryQty):
      correctSet = set(bruteForceFound[1][i])
      retArr = nmslibFound[i][0]
      retElem = retArr[n] if len(retArr) > n else -1

      recall = recall + int(retElem in correctSet)
    recall = recall / queryQty
    print('kNN recall for neighbor %d %f' % (n+1, recall))


def testRandomUnif(dataQty, queryQty, efS, dim, k):
  queryQty = min(dataQty, queryQty)
  dataMatrix = np.random.randn(dataQty, dim).astype(np.float32)
  indx = np.random.choice(np.arange(dataQty), size=queryQty, replace=False)
  queryMatrix = dataMatrix[indx, ].astype(np.float32)
  testHnswRecallL2(dataMatrix, queryMatrix, k, efS=efS)


def testRandomClustered(dataQty, centerQty, queryQty, efS, dim, k):
  queryQty = min(dataQty, queryQty)
  dataMatrix, _ = make_blobs(n_samples=dataQty, centers=centerQty, n_features=dim, random_state=0)
  dataMatrix = dataMatrix.astype(np.float32)
  indx = np.random.choice(np.arange(dataQty), size=queryQty, replace=False)
  queryMatrix = dataMatrix[indx, ].astype(np.float32)
  testHnswRecallL2(dataMatrix, queryMatrix, k, efS=efS)

testRandomClustered(100_000, centerQty=20, queryQty=1000, dim=100, k=10, efS=200)
testRandomUnif(100_000, 1000, dim=100, k=10, efS=200)
  
  
