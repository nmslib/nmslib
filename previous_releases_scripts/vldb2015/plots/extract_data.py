#!/usr/bin/env python
#
# A script that processes output file and
# produces a performance graph.
#
# Authors: Bilegsaikhan Naidan, Leonid Boytsov.
#
# This code is released under the
# Apache License Version 2.0 http://www.apache.org/licenses/.
#
#
#

import argparse
import os
import re
import sys
import itertools
import re
from subprocess import call

def parseHeader(row):
    h = {}
    for index, field in enumerate(row.rstrip().split('\t')):
        if field in h:
          raise Exception("Probably corrupt input file, a duplicate field: '" + field + "'")
        if index == 0:    # methodName is first field
            assert 'MethodName' == field
        h[field] = index
    return h

def clear(str):
    # replace all non alpha-numeric characters with spaces
    return re.sub(r'[^a-zA-Z0-9 ]', ' ', str)
    #return re.sub(r'\W+', '', methodName)

def parseExpr(inputFile, lineNumber, origRow, header):
    row = origRow.rstrip().split('\t')
    if len(row) != len(header):
      raise Exception("The input file '" + inputFile + "' is probably corrupt, as the number of values  in line "+str(lineNumber+1)+ " doesn't match the number of fields, expected # of fields: " + str(len(header)) + " but got: " + str(len(row)))
    
    name    = row[header['MethodName']]
    qty     = row[header['NumData']].strip() 
    recall  = row[header['Recall']] 

    return [name, qty, recall, origRow]

def doExtract(inputFile, outputFileName, qty, recall):
    header = {}
    closestElem = {}
    rows = open(inputFile).readlines()
    outputFile = open(outputFileName, 'w')
    order = []
    for lineNumber, row in enumerate(rows):
        if lineNumber == 0:   # header information
            header = parseHeader(row)
            outputFile.write(row)
        else:
            parsed = parseExpr(inputFile, lineNumber, row, header)
            rowQty = parsed[1].strip()
            if qty is not None:
              if qty != rowQty: continue

            if recall is None:
              outputFile.write(row)
              continue

            # group by method name
            methodName = parsed[0]
            key = (methodName, rowQty)

            rowRecall = float(parsed[2])
            parsed[2]=rowRecall

            if key in closestElem:
                elem = closestElem[key]
                if abs(elem[2] - recall) > abs(rowRecall-recall) :
                  closestElem[key] = parsed
            else:
                closestElem[key] = parsed
                order.append(key)
                

    #for key, dt in closestElem.items():
      #outputFile.write(dt[3])
    for o in order:
      outputFile.write(closestElem[o][3])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='extract specific entries')
    parser.add_argument('-i','--infile',  required=True, help='input file')
    parser.add_argument('-o','--outfile', required=True, help='output file')
    parser.add_argument('-q','--qty',     required=False, help='# of entries')
    parser.add_argument('-r','--recall',  required=False, help='select one option with the recall closest to a given one') 

    args = vars(parser.parse_args())

    inputFile  = args['infile']
    outputFile = args['outfile']
    qty        = args['qty'].strip()
    recall     = None
    if args['recall'] is not None: recall = float(args['recall'])

    print recall is None

    if qty is None and recall is None:
      raise Exception("Only one of the two parameters (recall and qty) can be None")
      

    doExtract(inputFile, outputFile, qty, recall)


