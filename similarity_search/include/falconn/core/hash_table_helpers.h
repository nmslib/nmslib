#ifndef __HASH_TABLE_HELPERS_H__
#define __HASH_TABLE_HELPERS_H__

#include "../falconn_global.h"

namespace falconn {
namespace core {

class HashTableError : public FalconnError {
 public:
  HashTableError(const char* msg) : FalconnError(msg) {}
};

}  // namespace core
}  // namespace falconn

#endif
