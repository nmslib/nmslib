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

#ifndef __LSHKIT_COMPOSITE__
#define __LSHKIT_COMPOSITE__

/**
 * \file composite.h
 * \brief Defining a bunch of LSH compositions.
 *
 * LSH composition is to use an existing LSH class as building
 * block to generate a new class of LSH.
 *
 * All template classes in this header are defined in a very similar fashion:
 *
 * \code
 * template <typename LSH> // base LSH is given as a template parameter
 * class SomeDerivedLSH
 * {
 * public:
 *      typedef LSH Super;  // the base LSH is defined to be Super.
 *      typedef ... Domain;
 *
 *      struct Parameter: public Super::Parameter {
 *          ...             // all parameters to the base LSH are inherited.
 *      };
 *      ...
 * };
 * \endcode
 *
 * 
 */

namespace lshkit {

///  The modulo operation on hash values.
/**
 *  The mod of an LSH function by some value N is usually still locality
 *  sensitive.  This can be used to limit the hash value of certain LSH,
 *  so that the hash value can be used to index a fixed-sized hash table.
 *
 *  Let LSH be the original class, and N be the divisor, then the parameter type
 *  is defined as
 *  \code
 *      struct Parameter {
 *          unsigned range;     // the divisor.
 *          ...                 // the original parameters are inherited.
 *      };
 * \endcode
 * The domain is the same as the original LSH and the range is always N.
 *
 */
template <typename LSH>
class Tail
{
    BOOST_CONCEPT_ASSERT((LshConcept<LSH>));
protected:
    LSH lsh_;
    unsigned range_;
public:
    /// The base LSH class.
    typedef LSH Super;
    /**
     * Parameter to Tail.
     */
    struct Parameter: public Super::Parameter {
        /// Desired range of the LSH.
        unsigned range;

        Parameter() : Super::Parameter(), range(0) {}
    };

    typedef typename Super::Domain Domain;

    Tail() {
    }

    template <typename RNG>
    void reset(const Parameter &param, RNG &rng) {
        range_ = param.range;
        lsh_.reset(param, rng);
    }

    template <typename RNG>
    Tail(const Parameter &param, RNG &rng) {
        reset(param, rng);
    }

    unsigned getRange () const {
        return range_;
    }

    unsigned operator () (Domain obj) const {
        return lsh_(obj) % range_;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & lsh_;
        ar & range_;
    }
};

/// The modulo operation with fixed divisor.
/**
 * This is the same as Tail, except the divisor is determined at compile time
 * and passed in as a template parameter.  Both the Domain and Parameter of the
 * original LSH are kept the same.
 */
template <typename LSH, unsigned range>
class FixedTail
{
    BOOST_CONCEPT_ASSERT((LshConcept<LSH>));
protected:
    LSH lsh_;
public:
    typedef LSH Super;

    /**
     * Parameter is exactly the same as the base class.
     */
    struct Parameter: public Super::Parameter {
    };

    typedef typename Super::Domain Domain;

    FixedTail()
    {
    }

    template <typename RNG>
    void reset(const Parameter &param, RNG &rng)
    {
        lsh_.reset(param, rng);
    }

    template <typename RNG>
    FixedTail(const Parameter &param, RNG &rng)
    {
        reset(param, rng);
    }

    unsigned getRange () const
    {
        return range;
    }

    unsigned operator () (Domain obj) const
    {
        return lsh_(obj) % range;
    }

    const LSH &getLsh () const
    {
        return lsh_;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & lsh_;
    }
};

/// Take the LSB of the hash value.
/**
 * This is a special case of FixedTail, with the divisor being 2, with the effect
 * of taking only the least significant bit of the hash value.  This is mainly used
 * to generate sketches.
 */
template <typename LSH>
class LSB: public FixedTail<LSH, 2>
{
public:
    typedef FixedTail<LSH,2> Super;
    /**
     * Parameter is exactly the same as the base class.
     */
    struct Parameter: public Super::Parameter {
    };

    typedef typename Super::Domain Domain;

    LSB () {}

    template <typename RNG>
    LSB(const Parameter &param, RNG &rng)
        : FixedTail<LSH, 2>(param, rng)
    {
    }
};

/// The DeltaLSH version of LSB..
/**
 * The original LSH must be a DeltaLSH.
 */
template <typename LSH>
class DeltaLSB: public FixedTail<LSH, 2>
{
    BOOST_CONCEPT_ASSERT((DeltaLshConcept<LSH>));

public:
    typedef FixedTail<LSH, 2> Super;
    /**
     * Parameter is exactly the same as the base class.
     */
    struct Parameter: public Super::Parameter {
    };

    typedef typename Super::Domain Domain;

    DeltaLSB() {}

    template <typename RNG>
    DeltaLSB(const Parameter &param, RNG &rng)
        : Super(param, rng)
    {
    }

    unsigned operator () (Domain obj) const
    {
        return Super::operator () (obj);
    }

    unsigned operator () (Domain obj, float *delta) const
    {
        float d;
        unsigned r = Super::getLsh()(obj, &d);
        *delta = min(d, 1.0F - d);
        return r % Super::getRange();
    }
};

/// Concatenation of a number of LSHes.
/**
 * The concatenation of a number of LSHes of the same class is usually used as a
 * new LSH to augment the locality sensitivity.  The Repeat class is to
 * concatenate N independent LSH instances.
 *
 * The domain of LSH remains the same.  The new parameter is 
 * defined as:
 *      \code
 *      struct Parameter {
 *          unsigned repeat;     // # of LSHes to concatenate.
 *          ...                  // all parameters of the base LSH are inherited.
 *      };
 *      \endcode
 *
 * Because the hash value is represented as unsigned int, which has only 32
 * bits, the range of the original LSH need to be small enough so that the
 * concatenated value does not overflow.  Specifically, we require that
 * \f[
 * LSH::getRange()^N <= 2^{32}.
 * \f]
 * We also require that the range of the base LSH only depends on the
 * parameter, so an array of such LSHes initialized with the same parameter but
 * independent random numbers have the same range.
 */
template <typename LSH>
class Repeat
{
    BOOST_CONCEPT_ASSERT((LshConcept<LSH>));

public:
    typedef LSH Super;
    /**
     * Parameter to Repeat.
     */
    struct Parameter: public Super::Parameter {
        /// The number of instances of the base LSH concatenated.
        unsigned repeat;

        Parameter() : Super::Parameter(), repeat(0) {}
    };

    typedef typename Super::Domain Domain;

    Repeat ()
    {
    }

    template <typename RNG>
    void reset(const Parameter &param, RNG &rng)
    {
        dup_ = param.repeat;
        assert(dup_ > 0);
        lsh_.resize(dup_);
        lsh_[0].reset(param, rng);
        range_ = unit_ = lsh_[0].getRange();
        assert(unit_ > 0);
        assert( (unsigned)(1 << (sizeof(unsigned) * 8 / dup_)) >= unit_);
        for (unsigned i = 1; i < dup_; ++i)
        {
            lsh_[i].reset(param, rng);
            assert(unit_ == lsh_[i].getRange());
            range_ *= unit_;
        }
    }

    template <typename RNG>
    Repeat(const Parameter &param, RNG &rng)
    {
        reset(param, rng);
    }

    unsigned getRange () const
    {
        return range_;
    }

    unsigned operator () (Domain obj) const
    {
        unsigned ret = 0;
        for (unsigned i = 0; i < dup_; ++i)
        {
            ret *= unit_;
            ret += lsh_[i](obj);
        }
        return ret;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & dup_;
        ar & range_;
        ar & unit_;
        ar & lsh_;
    }

protected:
    std::vector<Super> lsh_;
    unsigned dup_;
    unsigned range_;
    unsigned unit_;

};


/// Apply a random hash to the concatenation a number of hash values.
/**
 * This composition is to workaround the case where the range of individual
 * LSHes are so large that the concatenation can not be held in a single
 * unsgined int.  The method is to further hash the concatenated value.
 * Specifically, if <h1, h2, ..., hN> are the original values, this composition
 * produces (a1*h1 + a2*h2 + aN*hN), with a1~aN being random unsigned integers.
 * The range of the produced LSH is 0 (the whole range of unsigned).
 *
 * The domain of the LSH remains the same.  The new parameter is 
 * defined as:
 *      \code
 *      struct Parameter {
 *          unsigned repeat;     // # of LSHes to concatenate
 *          ...                  // all parameters of the base LSH are inherited.
 *      };
 *      \endcode
 */
template <typename LSH>
class RepeatHash
{
    BOOST_CONCEPT_ASSERT((LshConcept<LSH>));

public:
    typedef LSH Super;
    /**
     * Parameter to Repeat.
     */
    struct Parameter: public Super::Parameter {
        /// The number of instances of the base LSH concatenated.
        unsigned repeat;

        Parameter() : Super::Parameter(), repeat(0) {}
    };

    typedef typename LSH::Domain Domain;

    RepeatHash ()
    {
    }

    template <typename RNG>
    void reset(const Parameter &param, RNG &rng)
    {
        assert(param.repeat > 0);
        lsh_.resize(param.repeat);
        for (unsigned i = 0; i < param.repeat; ++i)
        {
            lsh_[i].reset(param, rng);
        }
        a_.resize(param.repeat);

        for (unsigned i = 0; i < param.repeat; ++i) a_[i] = rng();
    }

    template <typename RNG>
    RepeatHash(const Parameter &param, RNG &rng)
    {
        reset(param, rng);
    }

    unsigned getRange () const
    {
        return 0;
    }

    unsigned operator () (Domain obj) const
    {
        unsigned ret = 0;
        for (unsigned i = 0; i < lsh_.size(); ++i)
        {
            ret += lsh_[i](obj) * a_[i];
        }
        return ret;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & lsh_;
        ar & a_;
        assert(a_.size() == lsh_.size());
    }
protected:
    std::vector<Super> lsh_;
    std::vector<unsigned> a_;

};


///  XOR a number of 1-bit LSHes.
/*
 * The XOR of a number of 1-bit LSHes has higher locality sensitivity than
 * the original LSH.  This serves a similar purpose as RepeatHash.
 *
 * The new parameter is defined as:
 *      \code
 *      struct Parameter {
 *          unsigned xor_;     // # of LSHes to XOR
 *          ...                  // all parameters of the base LSH are inherited.
 *      };
 *      \endcode
 */
template <typename LSH>
class Xor
{
    BOOST_CONCEPT_ASSERT((LshConcept<LSH>));
public:

    typedef LSH Super;
    /**
     * Parameter to Xor
     */
    struct Parameter: public Super::Parameter {
        unsigned xor_; // xor is a keyword

        Parameter() : Super::Parameter(), xor_(0) {}
    };

    typedef typename Super::Domain Domain;

    Xor ()
    {
    }

    template <typename RNG>
    void reset(const Parameter &param, RNG &rng)
    {
        lsh_.resize(param.xor_);
        for (unsigned i = 0; i < lsh_.size(); ++i)
        {
            lsh_[i].reset(param, rng);
            assert(lsh_[i].getRange() == 2);
        }
    }

    template <typename RNG>
    Xor(const Parameter &param, RNG &rng)
    {
        reset(param, rng);
    }

    unsigned getRange () const
    {
        return 2;
    }

    unsigned operator () (Domain obj) const
    {
        unsigned ret = 0;
        for (unsigned i = 0; i < lsh_.size(); ++i)
        {
            ret = ret ^ lsh_[i](obj);
        }
        return ret;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & lsh_;
    }
protected:
    std::vector<Super> lsh_;
};

/// DeltaLSH version of XOR.
/*
 * This is essentially the same as XOR.  The delta of XOR is the
 * minimum of the deltas of all the original LSHes.
 */
template <typename LSH>
class DeltaXor: public Xor<LSH>
{
public:
    typedef Xor<LSH> Super;
    /**
     * Parameter to DeltaXor is exactly the same as Xor.
     */
    struct Parameter: public Super::Parameter  {
    };

    typedef typename Super::Domain Domain;

    DeltaXor ()
    {
    }

    template <typename RNG>
    DeltaXor(const Parameter &param, RNG &rng): Xor<LSH>(param, rng)
    {
    }

    unsigned operator () (Domain obj) const {
        return Super::operator () (obj);
    }

    unsigned operator () (Domain obj, float *delta) const
    {
        unsigned ret = 0;
        float m = std::numeric_limits<float>::max(), d;
        for (unsigned i = 0; i < Xor<LSH>::lsh_.size(); ++i)
        {
            ret = ret ^ Xor<LSH>::lsh_[i](obj, &d);
            m = min(m, d);
        }
        *delta = m;
        return ret;
    }
};

}

#endif

