/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2010--2013
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#ifndef _SPACE_BIT_HAMMING_H_
#define _SPACE_BIT_HAMMING_H_

#include <string>
#include <map>
#include <stdexcept>

#include <string.h>
#include "global.h"
#include "object.h"
#include "utils.h"
#include "space.h"
#include "distcomp.h"

#define SPACE_BIT_HAMMING "bit_hamming"

namespace similarity {

class SpaceBitHamming : public Space<int> {
 public:
  virtual ~SpaceBitHamming() {}

  virtual void ReadDataset(ObjectVector& dataset,
                      const ExperimentConfig<int>* config,
                      const char* inputfile,
                      const int MaxNumObjects) const;
  virtual Object* CreateObjFromVect(size_t id, const std::vector<uint32_t>& InpVect) const;
  virtual std::string ToString() const { return "Hamming (bit-storage) space"; }
 protected:
  virtual int HiddenDistance(const Object* obj1, const Object* obj2) const;
  void ReadVec(std::string line, std::vector<uint32_t>& v) const;
};

}  // namespace similarity

#endif
