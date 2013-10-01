/* 
    Copyright (C) 2009 Wei Dong <wdong@princeton.edu>. All Rights Reserved.
  
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

#ifndef __LSHKIT_VQ__
#define __LSHKIT_VQ__


#include <fstream>

namespace lshkit {

class VQ
{
    unsigned dim;
    unsigned K;
    std::vector<float> means;

    void *tree;

    void init ();
    unsigned search (const float *obj) const;
    void free ();
public:
    // Nothing
    struct Parameter
    {
        unsigned K;
        unsigned dim;
        std::string path;
    };

    typedef const float * Domain;

    VQ (): tree(0)
    {
    }

    template <typename RNG>
    void reset(const Parameter &param, RNG &rng)
    {
        dim = param.dim;
        K = param.K;
        means.resize(dim * K);
        std::ifstream is(param.path.c_str(), std::ios::binary);
        is.read((char *)&means[0], means.size() * sizeof(float));
        BOOST_VERIFY(is);
        init();
    }

    template <typename RNG>
    VQ(const Parameter &param, RNG &rng) : tree(0)
    {
        reset(param, rng);
    }

    ~VQ () {
        free();
    }

    unsigned getRange () const
    {
        return K;
    }

    unsigned operator () (Domain obj) const
    {
        return search(obj);
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & dim;
        ar & K;
        ar & means;
    }
};

}

#endif
