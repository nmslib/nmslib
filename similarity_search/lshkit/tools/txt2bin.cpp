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

/**
 * \file txt2bin.cpp
 * \brief Convert a dataset file from text to binary.
 *
 * Usage: txt2bin <input> <output>
 *
 * Example input file:
 \verbatim
 1 0 1 2
 2 3 4 5
 0.5 0.4 0.6 0.1
 \endverbatim
 *
 * Each row of the text file must contain the same number of columns.  Columns are
 * separated by single spaces or tabs.
 *
 */
#include <cctype>
#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <boost/foreach.hpp>
#include <boost/format.hpp>

using namespace std;
using namespace boost;

int main (int argc, char *argv[])
{
    unsigned size = sizeof(float), col, row, cnt;

    if (argc < 3)
    {
        cout << format("usage:\n\t%1% <in> <out>") % argv[0] << endl;
        return 0;
    }

    col = row = cnt = 0;

    ifstream is(argv[1]);
    // determine dimension (# columns)
    {
        string line;
        getline(is, line);
        is.seekg(0, ios::beg);

        bool is_blank = true;
        BOOST_FOREACH(char c, line) {
            if (isspace(c)) {
                is_blank = true;
            }
            else {
                if (is_blank) {
                    col++;
                }
                is_blank = false;
            }
        }
    }

    ofstream os(argv[2], ios::binary);
    os.seekp(sizeof(unsigned) * 3, ios::beg);
    for (;;)
    {
        float f;
        is >> f;
        if (!is) break;
        os.write((const char*)&f, sizeof(f));
        cnt++;
    }
    row = cnt / col;
    if (cnt % col != 0) throw runtime_error("FILE FORMAT ERROR");
    os.seekp(0, ios::beg);
    os.write((const char *)&size, sizeof(unsigned));
    os.write((const char *)&row, sizeof(unsigned));
    os.write((const char *)&col, sizeof(unsigned));
    return 0;

}
