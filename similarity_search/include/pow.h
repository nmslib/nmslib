/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2014
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#ifndef POW_HPP
#define POW_HPP

/*
 * An efficient computation of exponentiation to an INTEGER power.
 * See, Leonid's blog entry for details:
 *
 * http://searchivarius.org/blog/how-fast-are-our-math-libraries 
 *
 */

#include <cmath>

namespace similarity {

template <class T>
inline T EfficientPow(T Base, unsigned Exp) {
    if (Exp == 0) return 1;
    if (Exp == 1) return Base; 
    if (Exp == 2) return Base * Base; 
    if (Exp == 3) return Base * Base * Base; 
    if (Exp == 4) {
        Base *= Base;
        return Base * Base;
    }

    T res = Base;

    if (Exp == 5) {
        Base *= Base;
        return res * Base * Base;
    }

    if (Exp == 6) {
        Base *= Base;
        res = Base;
        Base *= Base;
        return res * Base;
    }

    if (Exp == 7) {
        Base *= Base;
        res *= Base;
        Base *= Base;
        return res * Base;
    }

    if (Exp == 8) {
        Base *= Base;
        Base *= Base;
        Base *= Base;
        return Base;
    }

    if (Exp == 9) {
        Base *= Base;
        Base *= Base;
        Base *= Base;
        return res * Base;
    }

    if (Exp == 10) {
        Base *= Base;
        res = Base;
        Base *= Base;
        Base *= Base;
        return res * Base;
    }

    if (Exp == 11) {
        Base *= Base;
        res  *= Base;
        Base *= Base;
        Base *= Base;
        return res * Base;
    }

    if (Exp == 12) {
        Base *= Base;
        Base *= Base;
        res = Base;
        Base *= Base;
        return res * Base;
    }

    if (Exp == 13) {
        Base *= Base;
        Base *= Base;
        res *= Base;
        Base *= Base;
        return res * Base;
    }

    if (Exp == 14) {
        Base *= Base;
        res = Base;
        Base *= Base;
        res *= Base;
        Base *= Base;
        return res * Base;
    }

    if (Exp == 15) {
        Base *= Base;
        res *= Base;
        Base *= Base;
        res *= Base;
        Base *= Base;
        return res * Base;
    }

    res *= res;
    res *= res;
    res *= res;
    res *= res;

    if (Exp == 16) {
        return res;
    }

    Exp -= 16;

    while (true) {
        if (Exp & 1) res *= Base;
        Exp >>= 1;
        if (Exp) {
            Base *= Base;
        } else {
            return res;
        }
    }

    return res;
}



}

template <class T>
inline T EfficientFractPowUtil(T Base, uint64_t Exp, uint64_t MaxK) {
    if (Exp == 0)    return 1;     // pow == 0 
    if (Exp == MaxK) return Base;  // pow == 1

    uint64_t Mask1 = MaxK - 1;
    uint64_t Mask2 = MaxK >>= 1;

    T res = 1.0;

    while (true) {
        Base = sqrt(Base);

        if (Exp & Mask2) res *= Base;

        Exp = (Exp << 1) & Mask1;

        if (!Exp) return res;
    }

    return res;
}

template <class T>
inline T EfficientFractPow(T Base, T FractExp, unsigned NumDig) {
    CHECK(FractExp <= 1 && NumDig);
    uint64_t MaxK = uint64_t(1) << NumDig;
    uint64_t Exp = static_cast<unsigned>(std::ceil(FractExp * MaxK));

    return EfficientFractPowUtil(Base, Exp, MaxK);
}


#endif
