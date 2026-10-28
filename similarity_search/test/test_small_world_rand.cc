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
#include <memory>

#include "bunit.h"
#include "knnquery.h"
#include "knnqueue.h"
#include "method/small_world_rand.h"
#include "spacefactory.h"

namespace similarity {

TEST(SmallWorldRandRepeatedAddBatch) {
  std::unique_ptr<Space<float>> space(
      SpaceFactoryRegistry<float>::Instance().CreateSpace("l2", AnyParams()));
  float values[] = {0, 100, 200, 300, 400};
  Object first(0, -1, sizeof(float), &values[0]);
  Object second(1, -1, sizeof(float), &values[1]);
  Object third(2, -1, sizeof(float), &values[2]);
  Object fourth(3, -1, sizeof(float), &values[3]);
  Object fifth(4, -1, sizeof(float), &values[4]);
  ObjectVector initial{&first, &second};

  for (int threads : {1, 2}) {
    SmallWorldRand<float> index(false, *space, initial);
    index.CreateIndex(AnyParams({"indexThreadQty=" + std::to_string(threads), "NN=5"}));
    index.SetQueryTimeParams(AnyParams({"efSearch=10"}));
    for (const auto& batch : {initial, ObjectVector{&third}, ObjectVector{&fourth, &fifth}}) {
      if (batch != initial) index.AddBatch(batch, false);
      for (const Object* object : batch) {
        KNNQuery<float> query(*space, object, 1);
        index.Search(&query, -1);
        EXPECT_EQ(object->id(), query.Result()->TopObject()->id());
      }
    }
  }
}

}  // namespace similarity
