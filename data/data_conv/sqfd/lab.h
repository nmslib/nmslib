/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2015
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#ifndef _LAB_H_
#define _LAB_H_

#include "global.h"

namespace sqfd {

Float3 RgbToXyz(const Float3& rgb);

Float3 XyzToLab(const Float3& xyz);

Float3 RgbToLab(const Float3& rgb);

Float3 XyzToRgb(const Float3& xyz);

Float3 LabToXyz(const Float3& lab);

Float3 LabToRgb(const Float3& lab);

float DeltaE(const Float3& lab1, const Float3& lab2);

}

#endif    // _LAB_H_
