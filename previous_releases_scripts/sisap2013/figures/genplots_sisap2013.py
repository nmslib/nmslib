#!/usr/bin/env python
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
from subprocess import call

AXIS = {
    'Recall': 'Recall',
    'RelPosError': 'Relative position error',
    'NumCloser': 'Number closer',
    'QueryTime': 'Query time (ms)',
    'DistComp': 'Distance computations',
    'ImprEfficiency': 'Improvement of efficiency',
    'ImprDistComp': 'Improvement of dist. computations',
    'Mem': 'Memory usage'
    }

def enum(*sequential, **named):
    enums = dict(zip(sequential, range(len(sequential))), **named)
    return type('Enum', (), enums)

AXIS_TYPES = enum('NORMAL', 'LOGX_NORMALY', 'NORMALX_LOGY', 'LOGLOG')

TITLE = {
    'k' : 'Performance for kNN search ({$k=%s$})',
    'r' : 'Performance for range search ({$r=%s$})'
}

LATEX = """
\\documentclass{article}
\\usepackage{pgfplots}
\\usepgfplotslibrary{external}
\\tikzexternalize[prefix=%s/]{%s}

\\begin{document}

  \\pgfplotsset{
     cycle multi list={
     black,blue,red\\nextlist
     solid,{dotted,mark options={solid}}\\nextlist
     mark=x,mark=square,mark=triangle,
     mark=o,mark=triangle*,mark=diamond*,mark=square*,mark=pentagon*
     },
     legend style={font=\\small},
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
                 xlabel=%s,ylabel=%s,title=%s,legend pos=%s,
                 tick label style={/pgf/number format/fixed},
             ]

             %s

          %s
        \\end{tikzpicture}
  \\end{figure}
"""

LATEX_LINE = """
\\addplot table[x index=0, y index=1] {
%s
};\\addlegendentry{%s}
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

def genPGFPlot(experiments, searchParam, xAxisField, yAxisField, axisType, legendPos, printYaxis, title):
    global LATEX_FIGURE
    global LATEX_LINE
    global AXIS
    global TITLE

    #assert searchParam[0] in TITLE
    #title = '' #TITLE[searchParam[0]] % (searchParam[2:])

    lines = []
    for methodName, points in experiments.items():
        #print(methodName, '\n'.join(points))
        lines.append(LATEX_LINE % ('\n'.join(points), methodName))

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

    axisLatex = getAxisLatex(axisType)
    #print(''.join(lines))

    return LATEX_FIGURE % ('plot_' + searchParam, axisLatex[0], xAxisDescr, yAxisDescr, title, legendPos, ''.join(lines), axisLatex[1])

def parseHeader(row):
    h = {}
    for index, field in enumerate(row.split('\t')):
        assert field not in h
        if index == 0:    # methodName is first field
            assert 'MethodName' == field
        h[field] = index
    return h

def parseExpr(row, header, xAxisField, yAxisField):
    row = row.split('\t')
    assert len(row) == len(header)
    #  [MethodName, "X Y"]
    return [renameMethod(clear(row[0])), row[header[xAxisField]] + ' ' + row[header[yAxisField]]]

def genPlot(outputFile, searchParam, xAxisField, yAxisField, axisType, legendPos, printYaxis, title):
    header = {}
    experiments = {}
    rows = open(outputFile).readlines()
    for lineNumber, row in enumerate(rows):
        if lineNumber == 0:   # header information
            header = parseHeader(row)
            assert xAxisField in header
            assert yAxisField in header
        else:
            parsed = parseExpr(row, header, xAxisField, yAxisField)
            # group by method name
            if parsed[0] in experiments:
                experiments[parsed[0]].append(parsed[1])
            else:
                experiments[parsed[0]] = [parsed[1]]
    #print(experiments)

    #for methodName, points in experiments.items():
    #    points.sort()
    #    experiments[methodName] = points

    return genPGFPlot(experiments, searchParam, xAxisField, yAxisField, axisType, legendPos, printYaxis, title)


def genPlotsInDir(outputDir, resultDir, resultFile, xAxisField, yAxisField, axisType, legendPos, printYaxis, title):
    plots = ''
    for fileName in os.listdir(outputDir):
        if fileName.startswith('res_') and fileName.endswith('.dat'):
            searchParam = re.findall(r"[-+]?\d*\.\d+|\d+", fileName)
            assert len(searchParam) == 1
            searchParam = fileName[4].lower() + '_' + str(searchParam[0])    # k_100, r_0.2
            #print(fileName, searchParam)
            plots = plots + '\n' + genPlot(os.path.join(outputDir, fileName), searchParam, xAxisField, yAxisField, axisType, legendPos, printYaxis, title)

    #print(plots)

    if not os.path.exists(resultDir):
        os.makedirs(resultDir)

    fp = open(resultFile + '.tex', 'w')
    latex = LATEX % (resultDir, resultFile, plots)
    fp.write(latex)
    fp.close()

    call(['pdflatex', '-shell-escape', resultFile])
    call(['rm', '-f', '*.{aux,auxlock,log}'])
    call(['rm', '-f', resultDir + '/*.{dep,dpth,log}'])

def renameMethod(methodName):
    methodName = methodName.strip()
    if methodName.find('vptree  triangle inequality') >= 0:
        return 'vp-tree'
    if methodName.find('permutation  incr  sorting') >= 0:
        return 'perm. incr.'
    if methodName.find('permutation  vptree') >= 0:
        return 'perm. vp-tree'
    if methodName.find('permutation  pref  index') >= 0:
        return 'perm. pref.'
    if methodName.find('multiprobe lsh') >= 0:
        return 'multi-probe LSH'
    if methodName.find('bbtree') >= 0:
        return methodName.strip()
    print >> sys.stderr, "Does not know how to rename the method '" + methodName + "'"
    exit(1)
    #assert False

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='plot generator')
    parser.add_argument('-d','--outdir',  help='experiments output directory')
    parser.add_argument('-y','--yaxis',   help='display the label for y axis?')
    parser.add_argument('-t','--title',   help='title')
    parser.add_argument('-p','--legpos',  help='positions of legends')

    args = vars(parser.parse_args())

    outputDir = args['outdir']
    if outputDir is None:
        parser.error('experiments output directory is required')
    printYaxis = args['yaxis']
    if printYaxis is None:
        parser.error('yaxis is required')
    title = args['title']
    if title is None:
        parser.error('title')

    if outputDir.endswith('/'):
        outputDir = outputDir[:-1]

    xAxisFields = ['NumCloser',     'NumCloser',    'RelPosError',    'RelPosError']
    yAxisFields = ['ImprEfficiency','ImprDistComp', 'ImprEfficiency', 'ImprDistComp']
    #axisTypes   = [AXIS_TYPES.NORMALX_LOGY, AXIS_TYPES.NORMALX_LOGY, AXIS_TYPES.NORMALX_LOGY]
    axisTypes   = [AXIS_TYPES.LOGLOG, AXIS_TYPES.LOGLOG, AXIS_TYPES.LOGLOG, AXIS_TYPES.LOGLOG]
    #legendPoss  = ['south east', 'north east', 'north east']
    #legendPoss  = ['north west', 'north west', 'north west', 'north west']
    #legendPoss  = ['north east', 'north east', 'north east', 'north east']
    legendPosStr = args['legpos']
    if legendPosStr is None:
        parser.error('legpos is required')
    legendPoss  = legendPosStr.split(',') 


    for xAxisField, yAxisField, axisType, legendPos in zip(xAxisFields, yAxisFields, axisTypes, legendPoss):
        resultFile = '%s_allplots_%s_%s' % (outputDir.split('/')[-1], xAxisField, yAxisField)
        resultDir = '%s_plots_%s_%s' % (outputDir.split('/')[-1], xAxisField, yAxisField)
        genPlotsInDir(outputDir, resultDir, resultFile, xAxisField, yAxisField, axisType, legendPos, printYaxis, title)


