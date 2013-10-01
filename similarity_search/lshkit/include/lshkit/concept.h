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

/**
 * \file concept.h
 * \brief Check the LSH concept.
 *
 *  An LSH class should define the following items to be used in the 
 *  LSHKIT framework:
 *
 *  - The parameter type:
 *      \code
 *      typedef ... Parameter;
 *      \endcode
 *  - The domain type (type that the LSH applies on):
 *      \code
 *      typedef ... Domain;
 *      \endcode
 *  - Default constructor:
 *      \code
 *      LSH()
 *      \endcode
 *  - Initialization method (same as LSH() immediately followed by reset()):
 *      \code
 *      void reset(const Parameter &, RNG &);
 *      \endcode
 *      The reset() function and on of the constructors take a random number
 *      generator.  This random number generator should not be further used after reset() or the constructor has returned.
 *  - Initializing constructor:
 *      \code
 *      LSH(const Parameter &, RNG &);  // equivalent to LSH() followed by reset()
 *      \endcode
 *  - A method that returns the range of hash value:
 *      \code
 *      unsigned getRange () const;
 *      \endcode
 *      If getRange() returns 0, then the hash value (returned by
 *      operator()) can be anything.  Otherwise, it can be from 0 to getRange() - 1.
 *  - An operator to apply the hash function:
 *      \code
 *      unsigned operator () (const Domain) const;
 *      \endcode
 *  - A serialization method following Boost serialization interface.
 *      \code
 *      template<class Archive>
 *      void serialize(Archive & ar, const unsigned int version)
 *      \endcode
 *
 *  Some of the LSH functions are created by rouding a real number to an integer,
 *  and the part rounded off (delta) usually carries useful information.
 *  Such LSH functions are modeled by the DeltaLSH concept,
 *  which should implement the following extra method:
 *
 *  - An operator to apply the LSH and also return the delta.
 *     \code
 *     unsigned operator () const (Domain, float *delta); 
 *     \endcode
*/

#ifndef __LSHKIT_CONCEPT__
#define __LSHKIT_CONCEPT__

#include <boost/concept/assert.hpp>
#include <boost/concept/usage.hpp>

namespace lshkit {

/// LSH concept checker.
template <typename LSH>
struct LshConcept
{
    typedef typename LSH::Domain Domain;
    typedef typename LSH::Parameter Parameter;

    BOOST_CONCEPT_USAGE(LshConcept)
    {
        LSH lsh1(param, rng1);
        LSH lsh2(param, rng2);
        LSH lsh3;
        lsh.reset(param, rng1);
        lsh.reset(param, rng2);
        same_type(lsh.getRange(), u);
        same_type(lsh(object), u);
        // current we do not check serialization.
    }
protected:
    boost::mt11213b rng1;
    boost::mt19937 rng2;
    LSH lsh;
    Domain object;
    const Parameter param;

    unsigned u;
    template <typename T>
    void same_type(T const &, T const&);
};

/// DeltaLSH concept checker.
template <typename LSH>
struct DeltaLshConcept: public LshConcept<LSH>
{
    typedef LshConcept<LSH> Super;
    BOOST_CONCEPT_USAGE(DeltaLshConcept)
    {
        float delta;
        Super::same_type(Super::lsh(Super::object, &delta), Super::u);
    }
};

}

#endif

