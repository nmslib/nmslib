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

#include <cassert>
#include <queue>
#include <lshkit/common.h>
#include <lshkit/vq.h>
#include "kdtree.h"

namespace lshkit
{
    void VQ::init () {
        if (tree != 0) free();
        tree = kd_tree_alloc(K, dim);
        kd_tree_index((kd_tree_t *)tree, &means[0]);
    }

    void VQ::free () {
        kd_tree_free((kd_tree_t *)tree);
    }

    unsigned VQ::search (Domain obj) const {
        unsigned cnt;
        return kd_tree_search((kd_tree_t *)tree, obj, &cnt);
    }
}

