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

AXIS = {
    'Frac': 'Fraction of data retrieved',
    'Recall': 'Recall',
    'RelPosError': 'Relative position error',
    'NumCloser': 'Number closer',
    'QueryTime': 'Query time (ms)',
    'DistComp': 'Distance computations',
    'ImprEfficiency': 'Improv. in efficiency',
    'ImprDistComp': 'Reduction in dist. comput.',
    'Mem': 'Memory usage',
    'NumData': '\\# of data points'
    }

AXIS_DESC = 'Three tilde separated values: (0|1);(norm|log);<metric name>, the first one is 1 if we need to print an axis, the second one chooses either a regular or a logarithmically transformed axis, the third one specifies the name of the metric, which is one of the following: ' + ','.join(AXIS.keys())
LEGENDS  = [ "north west", "north east", "south west", "south east" ]
LEGEND_DESC = 'Use "none" to disable legend generation. Otherwise, a legend is defined by two tilde separated values: <# of columns>,<legend position>. The legend position is either in the format (xPos,yPos)--try (1,-0.2)--or it can also be a list of relative positions from the list:' + ','.join(LEGENDS)

def enum(*sequential, **named):
    enums = dict(zip(sequential, range(len(sequential))), **named)
    return type('Enum', (), enums)

AXIS_TYPES = enum('NORMAL', 'LOGX_NORMALY', 'NORMALX_LOGY', 'LOGLOG')

LATEX = """
\\documentclass{article}
\\usepackage{pgfplots}
\\usepgfplotslibrary{external}
\\tikzexternalize{%s}
\\usetikzlibrary{plotmarks}

\\begin{document}

  \\pgfplotsset{
     title style={font=\\Large},
     %s
  }

  \\pgfsetplotmarksize{3pt}

%s
\\end{document}
"""

LATEX_FIGURE = """
  \\tikzsetnextfilename{%s}
  \\begin{figure}
       \\begin{tikzpicture}
          %s
             [
                 xlabel=%s,ylabel=%s,title=%s,
                 tick label style={/pgf/number format/fixed},
             ]

             %s

          %s
        \\end{tikzpicture}
  \\end{figure}
"""

LATEX_LINE = """
\\addplot [%s] table[x index=0, y index=1] {
%s
};%s
"""

def clear(str):
    # replace all non alpha-numeric characters with spaces
    return re.sub(r'[^a-zA-Z0-9 ]', ' ', str)
    #return re.sub(r'\W+', '', methodName)

def getAxisLatex(axisType):
    if axisType == AXIS_TYPES.NORMAL:
        return ['     \\begin{axis}', '    \\end{axis}']
    if axisType == AXIS_TYPES.LOGX_NORMALY:
        return ['     \\begin{semilogxaxis}', '    \\end{semilogxaxis}']
    # The xmin/max is hardcoded, because this plot is used only to display recall
    if axisType == AXIS_TYPES.NORMALX_LOGY:
        return ['     \\begin{semilogyaxis}[xmin=-0.05,xmax=1.05]', '    \\end{semilogyaxis}']
    if axisType == AXIS_TYPES.LOGLOG:
        return ['     \\begin{loglogaxis}', '    \\end{loglogaxis}']
    assert False

def genPGFPlot(experiments, methStyles, outputFile, xAxisField, yAxisField, axisType, noLegend, printXaxis, printYaxis, title):
    global LATEX_FIGURE
    global LATEX_LINE
    global AXIS
    global TITLE

    lines = []
    #for methodName, points in experiments.items():
    for methodName, points in experiments:
        legendEntry=''
        if not noLegend:
          legendEntry='\\addlegendentry{%s}' % methodName
        lines.append(LATEX_LINE % (methStyles[methodName],'\n'.join(points), legendEntry))

    if axisType == AXIS_TYPES.LOGX_NORMALY or axisType == AXIS_TYPES.LOGLOG:
        xAxisDescr = '%s (log. scale)' % AXIS[xAxisField]
    else:
        xAxisDescr = AXIS[xAxisField]

    if axisType == AXIS_TYPES.NORMALX_LOGY or axisType == AXIS_TYPES.LOGLOG:
        yAxisDescr = '%s (log. scale)' % AXIS[yAxisField]
    else:
        yAxisDescr = AXIS[yAxisField]
  
    if printYaxis == '0':
        yAxisDescr=''
    if printXaxis == '0':
        xAxisDescr=''

    axisLatex = getAxisLatex(axisType)
    #print(''.join(lines))

    return LATEX_FIGURE % (outputFile, axisLatex[0], xAxisDescr, yAxisDescr, title, ''.join(lines), axisLatex[1])

def parseHeader(row):
    h = {}
    for index, field in enumerate(row.rstrip().split('\t')):
        if field in h:
          raise Exception("Probably corrupt input file, a duplicate field: '" + field + "'")
        if index == 0:    # methodName is first field
            assert 'MethodName' == field
        h[field] = index
    return h

def parseExpr(inputFile, lineNumber, row, header, xAxisField, yAxisField, K):
    row = row.rstrip().split('\t')
    if len(row) != len(header):
      raise Exception("The input file '" + inputFile + "' is probably corrupt, as the number of values  in line "+str(lineNumber+1)+ " doesn't match the number of fields, expected # of fields: " + str(len(header)) + " but got: " + str(len(row)))

    NumData = int(row[12]) 
    knnAmp = 0
    projType = ''
    projDim = 0
    for tmp in row[11].replace('"','').split(','):
      (n,v) = tmp.split('=')
      if n == 'projDim': projDim = int(v)
      if n == 'projType': projType = v
      if n == 'knnAmp': knnAmp = int(v)
    scanFrac = float(knnAmp*K)/NumData
    #print "pt{} pd={} ka={}".format(projType,projDim,scanFrac)

    methodName = '{}-{}'.format(projType, projDim)
    
    #props  = methodNameAndStyle(clear(row[0]))
    props  = methodNameAndStyle(methodName)
    #return [props[0], props[1], row[header[xAxisField]] + ' ' + row[header[yAxisField]]]
    return [props[0], props[1], row[header[xAxisField]] + ' ' + str(scanFrac)]

def genPlotLatex(inputFile, outputFile, xAxisField, yAxisField, axisType, noLegend, printXaxis, printYaxis, title,K):
    header = {}
    experiments = {}
    expName = {}
    methStyles = {}
    rows = open(inputFile).readlines()
    num=0
    for lineNumber, row in enumerate(rows):
        if lineNumber == 0:   # header information
            header = parseHeader(row)
            if xAxisField not in header:
              raise Exception("You specified an invalid xAxis name '" + xAxisField + "', valid are: " + ','.join(header))
            if yAxisField != 'Frac' and yAxisField not in header:
              raise Exception("You specified an invalid yAxis name '" + yAxisField + "', valid are: " + ','.join(header))
        else:
            parsed = parseExpr(inputFile, lineNumber, row, header, xAxisField, yAxisField, K)
            # group by method name
            methodName = parsed[0]
            methodData = parsed[2]
            methStyles[methodName] = parsed[1]
            if methodName in experiments:
                experiments[methodName].append(methodData)
            else:
                experiments[methodName] = [methodData]
                expName[num] = methodName
                num = num + 1
  
    experiments_sorted = []
    for i in range(0,num): 
      t = (expName[i], experiments[expName[i]])
      experiments_sorted.append(t)

    return genPGFPlot(experiments_sorted, methStyles,outputFile, xAxisField, yAxisField, axisType, noLegend, printXaxis, printYaxis, title)


def genPlot(inputFile, outputFilePrefix, xAxisField, yAxisField, axisType, noLegend,legendNumColumn,legendRelative, legendPos, printXaxis, printYaxis, title,K):
    plots = genPlotLatex(inputFile, outputFilePrefix,  xAxisField, yAxisField, axisType, noLegend, printXaxis, printYaxis, title,K)

    legendDesc=' legend style={font=\\small} '

    if not noLegend:
      legendDesc = 'legend style={font=\\small},legend columns=' + legendNumColumn
      if legendRelative:
        legendDesc += ',legend pos=' + legendPos
      else:
        legendDesc += ',legend style={at={('+legendPos+')}}'
    outputFileName = outputFilePrefix + '.tex'
    fp = open(outputFileName, 'w')
    latex = LATEX % (outputFileName,legendDesc,plots)
    fp.write(latex)
    fp.close()

    call(['pdflatex', '-shell-escape', outputFilePrefix])
    call(['rm', '-f', '*.{aux,auxlock,log}'])
    call(['rm', '-f', outputFilePrefix + '/*.{dep,dpth,log}'])

# For the ease of reference, here are the marker lists (can be useful for future extensions).
# Other options (such as color and line type) can be specified,also a pgfplots manual for detail.
# Marker types (note that the prefix * denotes a solid shape and cannot be applied to some of the markers such as 'x'). 
# Note: Let's not use '|', because it is very similar to '+'
# * x + - o
# Also oplus* looks exactly as simply * 
#
# triangle square diamond 
# triangle* square* diamond* 
# oplus 
# otimes otimes*
# asterisk 
# pentagon  pentagon*
# text
# star

def startsWith(s, prefix):
  if len(s) >= len(prefix):
    return s[0:len(prefix)] == prefix
  return False 
     
def methodNameAndStyle(methodName):
    methodName = methodName.strip().lower()

    mark=''

    (projType, dim) = methodName.split('-')

    if   dim == '4'   : mark='*'
    if   dim == '8'   : mark='o'
    elif dim == '16'  : mark='x'
    elif dim == '32'  : mark='triangle'
    elif dim == '64'  : mark='pentagon'
    elif dim == '128' : mark='diamond'
    elif dim == '256' : mark='+'
    elif dim == '512' : mark='square'
    elif dim == '1024': mark='asterisk'

    if mark != '': return (projType+'-'+dim, 'mark='+mark)

    #if startsWith(methodName, 'vptree'):
    #    return ('vp-tree', 'mark=*')
    #if methodName == 'permutation  incr  sorting':
    #    return ('permutation incr.','mark=x')
    #if methodName == 'binarized permutation  vptree':
    #    return ('perm. bin. vptree','mark=+' )
    #if methodName == 'permutation  pref  index':
    #    return ('pref. index','mark=text')
    #if methodName == 'permutation  vptree':
    #    return ('perm. vptree','mark=diamond*')
    #if methodName == 'small world rand':
    #    return ('small world', 'mark=o')
    #if methodName == 'permutation  inverted index over neighboring pivots':
    #    return ('pivot neighb. index','mark=triangle')
    #if methodName == 'multiprobe lsh':
    #    return ('multi-probe LSH', 'mark=triangle*')
    #if methodName.find('copies of') >= 0:
    #    return (methodName.strip(), 'mark=square')
    #if methodName == 'bbtree':
    #    return (methodName.strip(), 'mark=square*')
    #if methodName == 'list of clusters':
    #    return ('list clust', 'mark=diamond')
    #if methodName == 'ghtree':
    #    return ('gh-tree', 'mark=diamond*')
    #if methodName == 'mvp tree':
    #    return ('mvp-tree', 'mark=oplus')
    #if methodName == 'satree':
    #    return ('sa-tree', 'mark=otimes')
    #if methodName == 'lsh':
    #    return ('LSH', 'mark=otimes*')
    #if methodName == 'permutation  inverted index':
    #    return ('perm inv index', 'mark=asterisk')
    #if methodName == 'permutation binarized  incr  sorting':
    #    return ('perm bin incr', 'mark=pentagon')
    #if methodName == 'sequential search':
    #    return ('brute force', 'mark=pentagon*')
    print >> sys.stderr, "Does not know how to rename the method '" + methodName + "'"
    exit(1)
    #assert False

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='plot generator')
    parser.add_argument('-i','--infile',  required=True, help='input file')
    parser.add_argument('-o','--outfile_pref', required=True, help='output file prefix')
    parser.add_argument('-x','--xaxis',   required=True, help='x axis description in the format: ' + AXIS_DESC)
    parser.add_argument('-y','--yaxis',   required=True, help='y axis description in the format: ' + AXIS_DESC)
    parser.add_argument('-l','--legend',  required=True, help='legend description: ' + LEGEND_DESC)
    parser.add_argument('-t','--title',   required=True, help='title')
    parser.add_argument('-k','--knn',   required=True, help='k in the k-NN')

    args = vars(parser.parse_args())

    inputFile  = args['infile']
    outputFilePrefix = args['outfile_pref']
    title      = args['title']
    legendDesc  = args['legend']
    K = int(args['knn'])

    noLegend = False
    if legendDesc == "none":
      noLegend = True
      legendNumColumn = None
      legendPos=None
      legendRelative=None
    else:
      tmp = legendDesc.split('~')

      if len(tmp) != 2:
        parser.error('Wrong format for legend, should be: ' + LEGEND_DESC)

      legendNumColumn = tmp[0]
      legendPos=tmp[1]

      legendRelative = True
      if re.match('^\([0-9.-]+,[0-9.-]+\)$', legendPos) is None:
        if legendPos not in LEGENDS: 
          parser.error('Unrecognized legend option, should be:' + LEGEND_DESC)
        legendRelative = True
      else:
        legendRelative = False

    tmpx = args['xaxis'].split('~')
    tmpy = args['yaxis'].split('~')

    if len(tmpx) != 3:
      parser.error('Wrong format for xaxis, should be: ' + AXIS_DESC)
    if len(tmpy) != 3:
      parser.error('Wrong format for yaxis, should be: ' + AXIS_DESC)

    (printXaxis,xt,xAxisField) = tmpx
    (printYaxis,yt,yAxisField) = tmpy

    if xt == 'norm' and yt == 'norm':
      axisType = AXIS_TYPES.NORMAL  
    elif xt == 'log' and yt == 'norm':
      axisType = AXIS_TYPES.LOGX_NORMALY  
    elif xt == 'norm' and yt == 'log':
      axisType = AXIS_TYPES.NORMALX_LOGY  
    elif xt == 'log' and yt == 'log':
      axisType = AXIS_TYPES.LOGLOG
    else:
      parser.error('Wrong format for x or y axis description, should be: ' + AXIS_DESC)

    genPlot(inputFile, outputFilePrefix, xAxisField, yAxisField, axisType, noLegend, legendNumColumn,legendRelative, legendPos, printXaxis, printYaxis, title, K)


