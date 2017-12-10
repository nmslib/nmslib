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

AXIS = {}

AXIS_DESC = 'Three tilde separated values: (0|1), (norm|log), <metric name>. The first one is 1 if we need to print an axis, the second one chooses either a regular or a logarithmically transformed axis, the third one specifies the name of the metric, which is one of the following: ' + ','.join(AXIS.keys())
LEGENDS  = [ "north west", "north east", "south west", "south east" ]
LEGEND_DESC = 'Use "none" to disable legend generation. Otherwise, a legend is defined by two tilde separated values: <# of columns>,<legend position>. The legend position is either in the format (xPos,yPos)--try (1,-0.2)--or it can also be a list of relative positions from the list:' + ','.join(LEGENDS)

METH_DESC={}

def fatalError(msg):
  print >> sys.stderr, msg
  exit(1)

def enum(*sequential, **named):
    enums = dict(zip(sequential, range(len(sequential))), **named)
    return type('Enum', (), enums)

AXIS_TYPES = enum('NORMAL', 'LOGX_NORMALY', 'NORMALX_LOGY', 'LOGLOG')

METHOD_ID_FIELD=""
METHOD_ID_FIELD_ID=None
AXIS_LIMITS=""
TITLE=""

LATEX = """
\\documentclass{article}
\\usepackage{pgfplots}
\\usepgfplotslibrary{external}
\\tikzexternalize{%s}
\\usetikzlibrary{plotmarks}

\\begin{document}

  \\pgfplotsset{
     title style={font=\\Large},
     tick label style={font=\\scriptsize},
     tick label style={/pgf/number format/fixed},
     label style={font=\\small},
     %s
  }

  \\pgfsetplotmarksize{2pt}

%s
\\end{document}
"""

LATEX_FIGURE = """
  \\tikzsetnextfilename{%s}
  \\begin{figure}
       \\begin{tikzpicture}
          %s
             [
                 xlabel=%s,ylabel=%s,%s%s
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
    if axisType == AXIS_TYPES.NORMALX_LOGY:
        return ['     \\begin{semilogyaxis}', '    \\end{semilogyaxis}']
    if axisType == AXIS_TYPES.LOGLOG:
        return ['     \\begin{loglogaxis}', '    \\end{loglogaxis}']
    assert False

def genPGFPlot(experiments, methNames, methStyles, outputFile, xAxisField, yAxisField, axisType, noLegend, printXaxis, printYaxis):
    global LATEX_FIGURE
    global LATEX_LINE
    global AXIS
    global AXIS_LIMITS
    global TITLE

    if xAxisField not in AXIS: 
      fatalError('Field %s is not defined in the AXIS-description file!' % xAxisField)
    if yAxisField not in AXIS: 
      fatalError('Field %s is not defined in the AXIS-description file!' % yAxisField)

    sortedMeths = []
    for methodId, methodData in experiments.items():
      sortedMeths.append((methNames[methodId], methodId))
    sortedMeths.sort()

    lines = []
    prevMethName=''
    for printSameMeth in [False, True]:
      for (methodName, methodId) in sortedMeths:
        legendEntry=''

        sameMethFlag = prevMethName == methodName
        prevMethName=methodName

        if not noLegend and not sameMethFlag:
          legendEntry='\\addlegendentry{%s}' % methodName

        if printSameMeth == sameMethFlag:
          methodData = experiments[methodId]
          methodData.sort()
          methodDataStr = [ str(e[0]) + ' ' + str(e[1]) for e in methodData] 
          lines.append(LATEX_LINE % (methStyles[methodId],'\n'.join(methodDataStr), legendEntry))
          lines.append('%s %s %s' % ('%', methodId, methodName))


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

    return LATEX_FIGURE % (outputFile, axisLatex[0], xAxisDescr, yAxisDescr, TITLE, AXIS_LIMITS, ''.join(lines), axisLatex[1])

# A very stupid escape function, let's not use it
def stupidEscape(str):
  return re.sub(r'([_#])', r'\\\1', str)

def parseHeader(row):
    global METHOD_ID_FIELD    
    global METHOD_ID_FIELD_ID
    h = {}
    for index, field in enumerate(row.rstrip().split('\t')):
        if field in h:
          fatalError("Probably corrupt input file, a duplicate field: '" + field + "'")
        if field == METHOD_ID_FIELD:
          METHOD_ID_FIELD_ID=index
        h[field] = index
    if METHOD_ID_FIELD_ID is None:
      fatalError("No field '%s' in the header!" % METHOD_ID_FIELD)
    return h

def parseExpr(inputFile, lineNumber, row, header, xAxisField, yAxisField):
    global METHOD_ID_FIELD_ID
    row = row.rstrip().split('\t')
    if len(row) != len(header):
      fatalError("The input file '" + inputFile + "' is probably corrupt, as the number of values  in line "+str(lineNumber+1)+ " doesn't match the number of fields, expected # of fields: " + str(len(header)) + " but got: " + str(len(row)))

    methodId = row[METHOD_ID_FIELD_ID]
    props  = methodNameAndStyle(methodId)
    return [methodId, props[0], props[1], (row[header[xAxisField]], row[header[yAxisField]]) ]

def genPlotLatex(inputFile, outputFile, xAxisField, yAxisField, axisType, noLegend, printXaxis, printYaxis):
    header = {}
    experiments = {}
    methStyles = {}
    methNames = {}
    rows = open(inputFile).readlines()
    for lineNumber, row in enumerate(rows):
        if row.strip() == '' or (len(row)>0 and row[0] == '#') : continue
        if lineNumber == 0:   # header information
            header = parseHeader(row)
            if xAxisField not in header:
              fatalError("You specified an invalid xAxis name '" + xAxisField + "', valid are: " + ','.join(header))
            if yAxisField not in header:
              fatalError("You specified an invalid yAxis name '" + yAxisField + "', valid are: " + ','.join(header))
        else:
            parsed = parseExpr(inputFile, lineNumber, row, header, xAxisField, yAxisField)
            # group by method name
            methodId   = parsed[0]
            methNames[methodId]   = parsed[1]
            methStyles[methodId]  = parsed[2]
            methodData = parsed[3]

            if methodId in experiments:
                experiments[methodId].append(methodData)
            else:
                experiments[methodId] = [methodData]

    return genPGFPlot(experiments, methNames, methStyles, outputFile, xAxisField, yAxisField, axisType, noLegend, printXaxis, printYaxis)


def genPlot(inputFile, outputFilePrefix, xAxisField, yAxisField, axisType, noLegend,legendNumColumn,legendRelative, legendPos, printXaxis, printYaxis):
    plots = genPlotLatex(inputFile, outputFilePrefix,  xAxisField, yAxisField, axisType, noLegend, printXaxis, printYaxis)

    legendDesc=' legend style={font=\\scriptsize} '

    if not noLegend:
      legendDesc = 'legend style={font=\\scriptsize},legend columns=' + legendNumColumn
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

def methodNameAndStyle(methodName):
  methodName = methodName.strip()
  desc=''
  mark=''
  if methodName in METH_DESC: return METH_DESC[methodName]
  methodNameCLR=clear(methodName)
  if methodNameCLR in METH_DESC: return METH_DESC[methodNameCLR]
  print >> sys.stderr, "Couldn't find the description of method '" + methodName + "' or '" + methodNameCLR + "'"
  exit(1)

# For the ease of reference, here are the marker lists (can be useful for future extensions).
# Other options (such as color, line type, and arrow head type) can be specified,also a pgfplots manual for detail.
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

def readAxisDesc(fn):
  res = {}
  f=open(fn)
  ln=0
  for line in f:
    ln=ln+1
    line=line.rstrip()
    if line == '': continue
    fields = line.split('\t')
    if len(fields) != 2: 
      fatalError('Wrong format of file %s line %d, expecting two TAB-separated fields' % (fn, ln))
    res[fields[0].strip()]=fields[1].strip()
  return res

def readMethDesc(fn) :
  res = {}
  f=open(fn)
  ln=0
  for line in f:
    ln=ln+1
    line=line.rstrip()
    if line == '': continue
    fields = line.split('\t')
    if len(fields) != 3: 
      fatalError('Wrong format of file %s line %d, expecting three TAB-separated fields' % (fn, ln))
    res[fields[0].strip()]=(fields[1].strip(),fields[2].strip())

  return res
  

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='plot generator')
    parser.add_argument('-a', '--axis_desc',required=True, help='Axis description file, where each line has two TAB-separated values: <axis name> and <axis label on the plot>')
    parser.add_argument('-m', '--meth_desc', required=True, help='Method description file, where each line has three TAB-separated values: <method label>, <method name for the plot>, and <pgfplot line description, e.g., mark=x>. See a pgfplots manual for the list of line marks, types, and colors, arrow heads.')
    parser.add_argument('-n', '--meth_id_field', required=True, help='The name of the field that defines method IDs/labels')
    parser.add_argument('-i', '--infile',  required=True, help='input file')
    parser.add_argument('-o', '--outfile_pref', required=True, help='output file prefix')
    parser.add_argument('-x', '--xaxis',   required=True, help='x axis description in the format: ' + AXIS_DESC)
    parser.add_argument('-y', '--yaxis',   required=True, help='y axis description in the format: ' + AXIS_DESC)
    parser.add_argument('-l', '--legend',  required=True, help='legend description: ' + LEGEND_DESC)

    parser.add_argument('-t', '--title',   required=False, help='title')

    parser.add_argument( '--xmin', required=False, help='The minimum x for the plot')
    parser.add_argument( '--ymin', required=False, help='The minimum y for the plot')
    parser.add_argument( '--xmax', required=False, help='The maximum x for the plot')
    parser.add_argument( '--ymax', required=False, help='The maximum y for the plot')

    args = vars(parser.parse_args())

    axisDescFile  = args['axis_desc']

    AXIS=readAxisDesc(axisDescFile)
    
    methDesc      = args['meth_desc']  

    METHOD_ID_FIELD=args['meth_id_field']

    METH_DESC=readMethDesc(methDesc)

    inputFile  = args['infile']
    outputFilePrefix = args['outfile_pref']
    legendDesc  = args['legend']

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

    xmin = args['xmin']
    ymin = args['ymin']
    xmax = args['xmax']
    ymax = args['ymax']

    if not xmin is None: AXIS_LIMITS = '%s,xmin=%s' % (AXIS_LIMITS, xmin)
    if not ymin is None: AXIS_LIMITS = '%s,ymin=%s' % (AXIS_LIMITS, ymin)
    if not xmax is None: AXIS_LIMITS = '%s,xmax=%s' % (AXIS_LIMITS, xmax)
    if not ymax is None: AXIS_LIMITS = '%s,ymax=%s' % (AXIS_LIMITS, ymax)

    title      = args['title']
    if not title is None: TITLE = 'title=%s' % title

    genPlot(inputFile, outputFilePrefix, xAxisField, yAxisField, axisType, noLegend, legendNumColumn,legendRelative, legendPos, printXaxis, printYaxis)


