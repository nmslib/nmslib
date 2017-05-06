/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2017
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#include <thread>
#include <queue>
#include <mutex>

namespace similarity {

  // See sample usage below
  template <class T>
  bool GetNextQueueObj(std::mutex &mtx, std::queue<T>& queue, T& obj) {
    std::unique_lock<std::mutex> lock(mtx);
    if (queue.empty()) {
      return false;
    }
    obj = queue.front();
    queue.pop();
    return true;
  }
/* 
   Sample usage of helper function GetNextQueueObj:
   
    queue<MSWNode*> toPatchQueue; // the job queue
    for (MSWNode* node : toPatchNodes) toPatchQueue.push(node);
    mutex mtx;
    vector<thread> threads;
    for (int i = 0; i < indexThreadQty_; ++i) {
      threads.push_back(thread(
          [&]() {
            MSWNode* node = nullptr;
            // get the next job from the queue
            while(GetNextQueueObj(mtx, toPatchQueue, node)) {
              node->removeGivenFriends(delNodesBitset);
            }
          }
      ));
    }
    // Don't forget to join!
    for (auto& thread : threads) thread.join();

*/
};
