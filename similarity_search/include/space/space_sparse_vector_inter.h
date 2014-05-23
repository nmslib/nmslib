/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2014
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#ifndef _SPACE_SPARSE_VECTOR_INTER_H_
#define _SPACE_SPARSE_VECTOR_INTER_H_

#include <cstdint>
#include <string>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <limits>

#include <string.h>
#include "global.h"
#include "object.h"
#include "utils.h"
#include "space.h"
#include "distcomp.h"
#include "space_sparse_vector.h"

namespace similarity {

using std::vector;

/*
 *
 * This helper base class is different from the SpaceSparseVector class 
 * in that it stores sparse vectors in a special format (divided into blocks).
 *
 * This makes it possible to quickly compute distances, 
 * where computations involves elements present in both vectors.
 *
 * One example is the scalar product.
 *
 */
template <typename dist_t>
class SpaceSparseVectorInter : public SpaceSparseVector<dist_t> {
 public:
  typedef SparseVectElem<dist_t> ElemType;


  /*
   * Need to override the function from the base class
   */
  virtual dist_t ScalarProduct(const Object* obj1, const Object* obj2) const {
    CHECK(obj1->datalength() > 0);
    CHECK(obj2->datalength() > 0);

    return ScalarProjectFast(obj1->data(), obj1->datalength(),
                             obj2->data(), obj2->datalength());
  }
 protected:

  virtual Object* CreateObjFromVect(size_t id, const vector<ElemType>& InpVect) const;
  
  virtual dist_t HiddenDistance(const Object* obj1, const Object* obj2) const = 0;

};

/* 
 *
 * Modifying ids, so we don't have ids for which id % 65536 == 0
 *
 * The SIMD intersection algorithm won't be able to handle such ids, 
 * because a faster version of _mm_cmpistrm requires that all ids 
 * are non-zero.
 *
 */
inline  size_t removeBlockZeros(size_t id) {
  // This way we don't have ids for which id % 65536 == 0
  return (id / 65535) * 65536 + (id % 65535) + 1;
};

/* 
 * A conversion that reverts removeBlockZeros
 * It works only for the numbers where id % 65536 == 0
 */
inline size_t addBlockZeros(size_t id) {
  return (id / 65536) * 65535 + (id % 65536) - 1;
}

template <typename dist_t>
inline  void ParseSparseElementHeader(const char*     pBuff, 
                                      size_t&         rBlockQty,
                                      dist_t&         rSqSum,
                                      const size_t*&  rpBlockQtys,
                                      const size_t*&  rpBlockOffs, 
                                      const char*&    rpBlockBegin) {
  const size_t*   pQty = reinterpret_cast<const size_t*>(pBuff);
  rBlockQty = *pQty;
  const dist_t*   pSqSum = reinterpret_cast<const dist_t*>(pQty + 1);
  rSqSum = *pSqSum;
  rpBlockQtys = reinterpret_cast<const size_t*>(pSqSum + 1);
  rpBlockOffs = rpBlockQtys + rBlockQty;

  rpBlockBegin = reinterpret_cast<const char*>(rpBlockOffs + rBlockQty);
}  

template <typename dist_t>
inline  void UnpackSparseElements(const char* pBuff, size_t dataLen,
                                  vector<SparseVectElem<dist_t>>& OutVect)
{
  typedef SparseVectElem<dist_t> ElemType;

  size_t            blockQty = 0;
  dist_t            SqSum = 0;
  const size_t*     pBlockQty = NULL;
  const size_t*     pBlockOff = NULL;

  const char* pBlockBegin = NULL;

  ParseSparseElementHeader(pBuff, blockQty, SqSum, 
                           pBlockQty, 
                           pBlockOff, 
                           pBlockBegin);

  size_t elemSize = 2 + sizeof(dist_t);

  for (size_t i = 0; i < blockQty; ++i) {
    size_t qty    = pBlockQty[i];
    size_t offset = pBlockOff[i];

    const uint16_t* pBlockIds = reinterpret_cast<const uint16_t*>(pBlockBegin);
    const dist_t* pValBegin = reinterpret_cast<const dist_t*>(pBlockIds + qty);

    for (size_t k = 0; k < qty; ++k) {
      OutVect.push_back(ElemType(addBlockZeros(pBlockIds[k] + offset), pValBegin[k]));
    }

    pBlockBegin += elemSize * qty;
  }

  CHECK(pBlockBegin - pBuff == (ssize_t)dataLen);
}

template <typename dist_t>
inline  void PackSparseElements(const vector<SparseVectElem<dist_t>>& InpVect, 
                                char*& prBuff, size_t& dataSize) {
  typedef SparseVectElem<dist_t> ElemType;

  vector<vector<ElemType>>    blocks;
  vector<size_t>              blockOffsets;

  uint32_t  prevBlockId = numeric_limits<uint32_t>::max();

  vector<ElemType> currBlock;

  size_t totQty = 0;
  dist_t sqSum = 0;

  for (size_t i = 0; i < InpVect.size(); ++i) {
	  uint32_t id = static_cast<uint32_t> // Blocks are never too large
						(removeBlockZeros(InpVect[i].id_));

	
    uint32_t blockId = static_cast<uint32_t> // Blocks are never too large
						(id / 65536);

    id &= 65535; // in-block id will contain only the last two bytes

    if (blockId != prevBlockId && !currBlock.empty()) {
      blocks.push_back(currBlock);
      blockOffsets.push_back(prevBlockId * 65536);
      currBlock.clear();
    }

    prevBlockId = blockId;
    currBlock.push_back(ElemType(id, InpVect[i].val_));
    sqSum += InpVect[i].val_ * InpVect[i].val_;
    ++totQty;
  }
  blocks.push_back(currBlock);
  blockOffsets.push_back(prevBlockId * 65536);

  CHECK(InpVect.size() == totQty);

  /* 
   * How much memory do we need?
   *
   * i)   A header will store the number of blocks. 
   * ii)  For each block we keep the # of elements
   * iii) The sum of squared element values.
   * iv)  Each element has a 2-byte id and a sizeof(dist_t) value
   *
   */
  size_t elemSize = 2 + sizeof(dist_t);
  dataSize = sizeof(size_t) + // number of blocks
              sizeof(dist_t) + // sum of squared elements
              2 * sizeof(size_t) * (blocks.size()) + // block qtys & offsets
              elemSize * InpVect.size();  

  prBuff = new char[dataSize]; 

  size_t*   pQty = reinterpret_cast<size_t*>(prBuff);

  /*
   * Store meta information.
   */
  *pQty = blocks.size(); 

  dist_t*   pSqSum = reinterpret_cast<dist_t*>(pQty + 1);
  *pSqSum = sqSum;
  size_t*   pBlockQtyOff = reinterpret_cast<size_t*>(pSqSum + 1);

  for (size_t i = 0; i < blocks.size(); ++i) {
    *pBlockQtyOff++ = blocks[i].size(); // qty
  }
  for (size_t i = 0; i < blocks.size(); ++i) {
    *pBlockQtyOff++ = blockOffsets[i]; // offset
  }
  /*
   * Store block data.
   */
  uint16_t* pBlockIds = reinterpret_cast<uint16_t*>(pBlockQtyOff);

  for (size_t i = 0; i < blocks.size(); ++i) {
    dist_t* pBlockVals = reinterpret_cast<dist_t*>(pBlockIds + blocks[i].size());

    for (size_t k = 0; k < blocks[i].size(); ++k) {
      *pBlockIds++ = blocks[i][k].id_;
      *pBlockVals++ = blocks[i][k].val_;
    }

    pBlockIds = reinterpret_cast<uint16_t*>(pBlockVals);
  }

  CHECK(reinterpret_cast<char*>(pBlockIds) - prBuff == (ssize_t)dataSize); 
}

}  // namespace similarity

#endif
