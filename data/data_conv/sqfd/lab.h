/**
 * Non-metric Space Library
 *
 * Main developers: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak
 *
 * For the complete list of contributors and further details see:
 * https://github.com/nmslib/nmslib
 *
 * Copyright (c) 2013-2018
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
