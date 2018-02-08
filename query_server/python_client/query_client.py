#!/usr/bin/env python
import sys, glob

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from protocol import QueryService
from protocol import ttypes

from datetime import datetime

import argparse

def error_exit(s):
  print("Error: %s" % s)
  sys.exit(1)

parser = argparse.ArgumentParser()

parser.add_argument('-p', '--port', help='TCP/IP server port number', action='store', type=int,required=True)
parser.add_argument('-a', '--addr', help='TCP/IP server address', action='store', required=True)
parser.add_argument('-k', '--knn', help='k for k-NN search', action='store', required=False,type=int)
parser.add_argument('-r', '--range', help='range for the range search', action='store', required=False,type=float)
parser.add_argument('-t', '--queryTimeParams', help='Query time parameter', action='store', default='')
parser.add_argument('-o', '--retObj', help='Return string representation of found objects?', action='store_true', default=False)
parser.add_argument('-e', '--retExternId', help='Return external IDs?', action='store_true', default=False)

args = parser.parse_args()

host=args.addr
port=args.port
queryTimeParams=args.queryTimeParams
retObj = args.retObj
retExternId = args.retObj

try:
  print("Host %s socket %d" % (host,port))
  # Make socket
  transport = TSocket.TSocket(host, port)
  # Buffering is critical. Raw sockets are very slow
  transport = TTransport.TBufferedTransport(transport)
  # Wrap in a protocol
  protocol = TBinaryProtocol.TBinaryProtocol(transport)

  client = QueryService.Client(protocol)

  transport.open()

  queryObj = '' 
  for s in sys.stdin:
    queryObj = queryObj + s + '\n'

  if args.queryTimeParams != '': 
    client.setQueryTimeParams(args.queryTimeParams)

  t1 = datetime.now()

  res = None
  if not args.knn is None:
    k = args.knn
    if not args.range is None:
      error_exit('Range search is not allowed if the KNN search is specified!')
    print("Running %d-NN search" % k)
    res = client.knnQuery(k, queryObj, retObj, retExternId)
  elif not args.range is None:
    r = args.range
    if not args.knn is None:
      error_exit('KNN search is not allowed if the range search is specified')
    print("Running range search, range=%f" % r)
    res = client.rangeQuery(r, queryObj, retObj, retExternId)
  else: 
    error_exit("Wrong search type %s" % searchType)

  t2 = datetime.now()

  diff = t2- t1

  elapsed = diff.seconds * 1e3 + diff.microseconds / 1e3

  print("Finished in %f ms" % elapsed)

  for e in res:
    s=''
    if retExternId:
      s='externId=' + e.externId
    print("id=%d dist=%f %s" % (e.id, e.dist, s))
    if retObj:
      print(e.obj)

# Close!
  transport.close()

except(Thrift.TException, tx):
  print('%s' % (tx.message))
