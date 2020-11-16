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
#include "lab.h"

namespace sqfd {

const float refX = 95.047;
const float refY = 100.000;
const float refZ = 108.883;

const float Kl = 1.0;
const float K1 = 0.045;
const float K2 = 0.015;

Float3 RgbToXyz(const Float3& rgb) {
  assert(rgb[0] >= 0 && rgb[0] <= 255);
  assert(rgb[1] >= 0 && rgb[1] <= 255);
  assert(rgb[2] >= 0 && rgb[2] <= 255);

  float r = rgb[0]/255.0;
  float g = rgb[1]/255.0;
  float b = rgb[2]/255.0;

  auto func = [](float v) {
    return (v > 0.04045 ? pow((v+0.055)/1.055, 2.4) : v/12.92) * 100.0f;
  };

  r = func(r);
  g = func(g);
  b = func(b);

  return Float3{{r * 0.4124f + g * 0.3576f + b * 0.1805f,
                 r * 0.2126f + g * 0.7152f + b * 0.0722f,
                 r * 0.0193f + g * 0.1192f + b * 0.9505f}};
}

Float3 XyzToLab(const Float3& xyz) {
  float x = xyz[0] / refX;
  float y = xyz[1] / refY;
  float z = xyz[2] / refZ;

  auto func = [](float v) {
    return v > 0.008856 ? pow(v, (1.0/3.0)) : (7.787*v + 16.0/116.0);
  };

  x = func(x);
  y = func(y);
  z = func(z);

  return Float3{{116.0f * y - 16.0f,
                 500.0f * (x - y),
                 200.0f * (y - z)}};
}

// rgb -> xyz -> lab
Float3 RgbToLab(const Float3& rgb) {
  return XyzToLab(RgbToXyz(rgb));
}

Float3 XyzToRgb(const Float3& xyz) {
  float x = xyz[0] / 100.0;
  float y = xyz[1] / 100.0;
  float z = xyz[2] / 100.0;

  float r = x * 3.2406f + y * -1.5372f + z * -0.4986f;
  float g = x * -0.9689f + y * 1.8758f + z * 0.0415f;
  float b = x * 0.0557f + y *-0.2040f + z * 1.0570f;

  auto func = [](float v) {
    return (v > 0.0031308 ? 1.055*pow(v,(1.0/2.4))-0.055 : 12.92*v) * 255.0f;
  };

  r = func(r);
  g = func(g);
  b = func(b);

  return Float3{{r, g, b}};
}

Float3 LabToXyz(const Float3& lab) {
  float y = (lab[0] + 16.0)/116.0;
  float x = lab[1] / 500 + y;
  float z = y - lab[2] / 200.0;

  auto func = [](float v) {
    return v*v*v > 0.008856 ? v*v*v : (v-16.0/116.0)/7.787;
  };

  x = refX * func(x);
  y = refY * func(y);
  z = refZ * func(z);

  return Float3{{x, y, z}};
}

Float3 LabToRgb(const Float3& lab) {
  return XyzToRgb(LabToXyz(lab));
}

float DeltaE(const Float3& lab1, const Float3& lab2) {
  float dl = lab1[0] - lab2[0];
  float da = lab1[1] - lab2[1];
  float db = lab1[2] - lab2[2];

  float c1 = sqrt(sqr(lab1[1]) + sqr(lab1[2]));
  float c2 = sqrt(sqr(lab2[1]) + sqr(lab2[2]));
  float dc = c1 - c2;

  float h = sqr(da) + sqr(db) - sqr(dc);
  h = h < 0 ? 0 : sqrt(h);

  float res = sqr(dl/Kl) + sqr(dc/(1.0+K1*c1)) + sqr(h/(1.0+K2*c1));
  return res < 0.0 ? 0.0 : sqrt(res);
}

}
