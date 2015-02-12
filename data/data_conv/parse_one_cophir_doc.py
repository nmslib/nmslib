#!/usr/bin/env python
import libxml2
from sys import argv

if len(argv) != 2:
  raise Exception("Usage: "+argv[0] + " <doc to parse> ")

doc = libxml2.parseFile(argv[1])
ct = doc.xpathNewContext()

vect = []

def GetList(elem, name):
  elem = str(elem)
  if (elem == ''): 
    raise Exception('Empty for ' + name)
  return elem.split()
 
vect.extend(GetList(ct.xpathEval('/SapirMMObject/Mpeg7/Description[@type="ContentEntityType"]/MultimediaContent[@type="ImageType"]/Image/VisualDescriptor[@type="ScalableColorType"]/Coeff/text()')[0], 'ScalableColorType'))
vect.extend(GetList(ct.xpathEval('/SapirMMObject/Mpeg7/Description[@type="ContentEntityType"]/MultimediaContent[@type="ImageType"]/Image/VisualDescriptor[@type="ColorStructureType" and @colorQuant="2"]/Values/text()')[0], 'ColorStructureType'))

for d in ct.xpathEval('/SapirMMObject/Mpeg7/Description[@type="ContentEntityType"]/MultimediaContent[@type="ImageType"]/Image/VisualDescriptor[@type="ColorLayoutType"]/*/text()'):
  vect.extend(GetList(d, 'ColorLayoutType'))

vect.extend(GetList(ct.xpathEval('/SapirMMObject/Mpeg7/Description[@type="ContentEntityType"]/MultimediaContent[@type="ImageType"]/Image/VisualDescriptor[@type="EdgeHistogramType"]/BinCounts/text()')[0], 'EdgeHistogramType'))
vect.extend(GetList(ct.xpathEval('/SapirMMObject/Mpeg7/Description[@type="ContentEntityType"]/MultimediaContent[@type="ImageType"]/Image/VisualDescriptor[@type="HomogeneousTextureType"]/Average/text()')[0], 'HomogeneousTextureType/Average'))
vect.extend(GetList(ct.xpathEval('/SapirMMObject/Mpeg7/Description[@type="ContentEntityType"]/MultimediaContent[@type="ImageType"]/Image/VisualDescriptor[@type="HomogeneousTextureType"]/StandardDeviation/text()')[0], 'HomogeneousTextureType/StandardDeviation'))
vect.extend(GetList(ct.xpathEval('/SapirMMObject/Mpeg7/Description[@type="ContentEntityType"]/MultimediaContent[@type="ImageType"]/Image/VisualDescriptor[@type="HomogeneousTextureType"]/Energy/text()')[0], 'HomogeneousTextureType/Energy'))
vect.extend(GetList(ct.xpathEval('/SapirMMObject/Mpeg7/Description[@type="ContentEntityType"]/MultimediaContent[@type="ImageType"]/Image/VisualDescriptor[@type="HomogeneousTextureType"]/EnergyDeviation/text()')[0], 'HomogeneousTextureType/EnergyDeviation'))

#print vect
#print "## LEN: ##" + str(len(vect))
if len(vect) != 282:
  Raise('Wrong #' + str(len) + " : " + ' '.join(vect))
print ' '.join(vect)

