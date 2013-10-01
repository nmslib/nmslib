#ifndef LSHKIT_MATRIX
#define LSHKIT_MATRIX
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

#include <fstream>
#include <boost/dynamic_bitset.hpp>

/**
 * \file matrix.h
 * \brief Dataset management.  A dataset is maintained as a matrix in memory.
 *
 * LSHKIT is a main memory indexing data structure and doesn't assume any
 * particular file format.  However, the standalone tools come with the LSHKIT
 * package use the following simple binary file format.
 *
 * The file contains N D-dimensional vectors of single precision floating point numbers.
 *
 * In the beginning of the file are three 32-bit unsigned integers: ELEM-SIZE,
 * SIZE, DIM. ELEM-SIZE is the size of the element, and currently the only
 * valid value is 4, which is the size of float.  SIZE is the number of vectors
 * in the file and DIM is the dimension.
 *
 * The rest of the file is SIZE vectors stored consecutively, and the total
 * size is SIZE * DIM * 4 bytes.
 *
 * Such binary files can be accessed using lshkit::Matrix<float>.
 */

namespace lshkit {

/// Matrix
/**
  * An matrix of size (NxD) is used to store an array of N D-dimensional
  * vectors.
  */
template <class T>
class Matrix
{
    int dim, N;
    T *dims;

    void load (const char *);
    void save (const char *);

#ifdef MATRIX_MMAP
    int fd;
#endif
public:
    /// Reset the size of matrix.
    /**
     * @param dim Dimension of each feature vector.
     * @param N Number of feature vectors.
     */
    void reset (int _dim, int _N)
    {
        dim = _dim;
        N = _N;
        if (dims != NULL) delete[] dims;
        dims = new T[dim * N];
    }

    /// Release memory.
    void free (void) {
        dim = N = 0;
        if (dims != NULL) delete[] dims;
        dims = NULL;
    }
    
    /// Default constructor.
    /** Allocates an empty matrix.  Should invoke reset or load before using it.*/
    Matrix () :dim(0), N(0), dims(NULL) {}

    /// Constructor, same as Matrix() followed immediately by reset().
    Matrix (int _dim, int _N) : dims(NULL) { reset(_dim, _N); }

    /// Destructor.
    ~Matrix () { if (dims != NULL) delete[] dims; }

    /// Access the ith vector.
    const T *operator [] (int i) const {
        return dims + i * dim;
    }

    /// Access the ith vector.
    T *operator [] (int i) {
        return dims + i * dim;
    }

    int getDim () const {return dim; }
    int getSize () const {return N; }


    /// Peek into a file to determine the size and dimension of the dataset.
    /**
     *  @param path File to peek.
     *  @param elem_size Size of the vector element.
     *  @param size Number of vectors in the file.
     *  @param dim Dimension of the vectors.
     *
     *  This function doesn't read the whole matrix into memory, so it is fast.
     */
    static void peek (const std::string &path, int *elem_size, int *size, int *dim);

    void load (const std::string &path);
    void save (const std::string &path);
    void load (std::istream &is);
    void save (std::ostream &os);

#ifdef MATRIX_MMAP
    void map (const std::string &path);
    void unmap ();
#endif

    /// Construct from a file.
    Matrix (const std::string &path): dims(NULL) { load(path); }

    /// An accessor class to be used with LSH index.
    class Accessor
    {
        const Matrix &matrix_;
        boost::dynamic_bitset<> flags_;
    public:
        typedef unsigned Key;
        typedef const float *Value;

        Accessor(const Matrix &matrix)
            : matrix_(matrix), flags_(matrix.getSize()) {}

        void reset () {
            flags_.reset();
        }

        bool mark (unsigned key) {
            if (flags_[key]) return false;
            flags_.set(key);
            return true;
        }

        const float *operator () (unsigned key) {
            return matrix_[key];
        }
    };

    /*
    class View
    {
        const Matrix<T> *ref_;
        int off_, len_;
    public:
        View (const Matrix<T> *ref, int off, int len)
            : ref_(ref), off_(off), len_(len) {}
        const T *operator [] (int i) const { return ref_->operator[](off_ + i); }
        int getDim () const {return ref_->getDim(); }
        int getSize () const {return len_; }
    };

    View getView (int off, int len) const
    {
        return View(this, off, len);
    }

    View getView () const
    {
        return View(this, 0, getSize());
    }
    */
};

typedef Matrix<float> FloatMatrix;

}

#include <lshkit/matrix-io.h>
#endif
