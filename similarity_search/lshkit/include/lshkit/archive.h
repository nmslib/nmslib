/* 
    Copyright (C) 2008 Wei Dong <wdong@princeton.edu>. All Rights Reserved.
  
    This file is part of LSHKIT.
  
    LSHKIT is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LSHKIT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with LSHKIT.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __LSHKIT_ARCHIVE__
#define __LSHKIT_ARCHIVE__

#include <vector>
#include <iostream>

/**
 * \file archive.h
 * \brief A set of binary I/O routines.
 *
 * The LSH library was originally designed to use boost serialization,
 * but it turned out that boost serialization is too generalized and too slow for us.
 * So we wrote our own minimal replacement.
 *
 * You are not expected to use these routines unless you are implementing your own
 * LSH class.
 * 
 * Given a variable V and a stream S, "S & V" will output V to S if S is derived from
 * std::ostream, or load V from S if S is derived from std::istream.  The
 * operator is defined for the following types:
 *  - classes where a "serialize" method is defined (required by
 * the LSH concept);
 *  - std::vector of such classes;
 *  - unsigned, float, std::vector<unsigned>, std::vector<float>.
 *
 * Note that binary I/O is architecture dependent and not portable.
 *
*/

namespace lshkit {

static inline std::ostream &operator & (std::ostream &os, int i) {
    return os.write((const char *)&i, sizeof(i));
}

static inline std::ostream &operator & (std::ostream &os, unsigned i) {
    return os.write((const char *)&i, sizeof(i));
}

static inline std::ostream &operator & (std::ostream &os, float i) {
    return os.write((const char *)&i, sizeof(i));
}

static inline std::istream &operator & (std::istream &is, int &i) {
    return is.read((char *)&i, sizeof(i));
}

static inline std::istream &operator & (std::istream &is, unsigned &i) {
    return is.read((char *)&i, sizeof(i));
}

static inline std::istream &operator & (std::istream &is, float &i) {
    return is.read((char *)&i, sizeof(i));
}

static inline std::ostream &operator & (std::ostream &os, std::vector<float> &v) {
    unsigned L = v.size();
    os & L;
    os.write((const char *)&v[0], v.size() * sizeof(float));
    return os;
}

static inline std::istream &operator & (std::istream &is, std::vector<float> &v) {
    unsigned L;
    is & L;
    v.resize(L);
    is.read((char *)&v[0], v.size() * sizeof(float));
    return is;
}

static inline std::ostream &operator & (std::ostream &os, std::vector<unsigned> &v) {
    unsigned L = v.size();
    os & L;
    os.write((const char *)&v[0], v.size() * sizeof(unsigned));
    return os;
}

static inline std::istream &operator & (std::istream &is, std::vector<unsigned> &v) {
    unsigned L;
    is & L;
    v.resize(L);
    is.read((char *)&v[0], v.size() * sizeof(unsigned));
    return is;
}

template <typename D>
std::ostream &operator & (std::ostream &s, D &t) {
    t.serialize(s, 0);
    return s;
}

template <typename D>
std::istream &operator & (std::istream &s, D &t) {
    t.serialize(s, 0);
    return s;
}

template <typename D>
std::ostream &operator & (std::ostream &os, std::vector<D> &v) {
    unsigned L = v.size();
    os & L;
    for (unsigned i = 0; i < L; ++i) {
        os & v[i];
    }
    return os;
}

template <typename D>
std::istream &operator & (std::istream &is, std::vector<D> &v) {
    unsigned L;
    is & L;
    v.resize(L);
    for (unsigned i = 0; i < L; ++i) {
        is & v[i];
    }
    return is;
}


}

#endif
