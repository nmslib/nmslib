/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_PLATFORM_DEFAULT_INTEGRAL_TYPES_H_
#define TENSORFLOW_PLATFORM_DEFAULT_INTEGRAL_TYPES_H_

// IWYU pragma: private, include "third_party/tensorflow/core/platform/types.h"
// IWYU pragma: friend third_party/tensorflow/core/platform/types.h

#include <cstdint>

namespace tensorflow {

typedef std::int8_t     int8;
typedef std::int16_t    int16;
typedef std::int32_t    int32;
typedef std::int64_t    int64;

typedef std::uint8_t    uint8;
typedef std::uint16_t   uint16;
typedef std::uint32_t   uint32;
typedef std::uint64_t   uint64;

}  // namespace tensorflow

#endif  // TENSORFLOW_PLATFORM_DEFAULT_INTEGRAL_TYPES_H_
