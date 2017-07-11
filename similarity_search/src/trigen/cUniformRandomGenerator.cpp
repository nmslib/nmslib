/*
 * Part of TriGen (Tomas Skopal, Jiry Novak)
 * Downloaded from http://siret.ms.mff.cuni.cz/skopal/download.htm
 */
/**************************************************************************}
{                                                                          }
{    cUniformRandomGenerator.cpp                          		      		}
{                                                                          }
{                                                                          }
{                 Copyright (c) 1998, 2003					Jiri Dvorsky		}
{                                                                          }
{    VERSION: 2.0														DATE 27/2/2003    }
{                                                                          }
{             following functionality:													}
{    Generator nahodnych cisel s rovnomernym rozlozenim.                   }
{    Algoritmus L'Ecuyer, delka periody > 2.10^18                          }
{    Zalozeno na kodu z knihy Numerical Recipes.                           }
{                                                                          }
{                                                                          }
{                                                                          }
{    UPDATE HISTORY:                                                       }
{                                                                          }
{                                                                          }
{**************************************************************************/
#include "trigen/cUniformRandomGenerator.h"


const int cUniformRandomGenerator::IM1 = 2147483563;
const int cUniformRandomGenerator::IM2 = 2147483399;
const double cUniformRandomGenerator::AM = (1.0/IM1);
const int cUniformRandomGenerator::IMM1 = (IM1-1);
const int cUniformRandomGenerator::IA1 = 40014;
const int cUniformRandomGenerator::IA2 = 40692;
const int cUniformRandomGenerator::IQ1 = 53668;
const int cUniformRandomGenerator::IQ2 = 52774;
const int cUniformRandomGenerator::IR1 = 12211;
const int cUniformRandomGenerator::IR2 = 3791;
const int cUniformRandomGenerator::NDIV = (1+IMM1/NTAB);
const double cUniformRandomGenerator::EPS = 1.2e-7;
const double cUniformRandomGenerator::RNMX = (1.0-EPS);


cUniformRandomGenerator::cUniformRandomGenerator(const bool Randomize):
	cAbstractRandomGenerator(), idum2(123456789), iy(0)
{
	int j, k;
	time_t t;
	if (Randomize)
	{
		t = time(NULL);
		if (t == 0)
			idum = 1;
		else
			idum = (int)(t < 0 ? -t : t);
	}
	else
	{
		idum = 1;
	}
	idum2 = idum;
	for(j = NTAB+7; j >= 0; j--)
	{
		k = idum/IQ1;
		idum = IA1*(idum - k*IQ1) - k*IR1;
		if (idum < 0)
			idum += IM1;
		if (j < NTAB)
			iv[j] = idum;
	}  // for
	iy = iv[0];
}


cUniformRandomGenerator::cUniformRandomGenerator(const int ARandSeed):
	cAbstractRandomGenerator(), idum2(123456789), iy(0)
{
	int j, k;
	if (ARandSeed == 0)
		idum = 1;
	else
		idum = ARandSeed > 0 ? ARandSeed : -ARandSeed;
	idum2 = idum;
	for(j = NTAB+7; j >= 0; j--)
	{
		k = idum/IQ1;
		idum = IA1*(idum - k*IQ1) - k*IR1;
		if (idum < 0)
			idum += IM1;
		if (j < NTAB)
			iv[j] = idum;
	}  // for
	iy = iv[0];
}


cUniformRandomGenerator::~cUniformRandomGenerator()
{
}

double cUniformRandomGenerator::GetNext()
{
	double temp;
	int j, k;
	k =idum/IQ1;
	idum = IA1*(idum - k*IQ1) - k*IR1;
	if (idum < 0)
		idum += IM1;
	k = idum2/IQ2;
	idum2 = IA2*(idum2-k*IQ2)-k*IR2;
	if (idum2 < 0)
		idum2 += IM2;
	j = iy/NDIV;
	iy = iv[j] - idum2;
	iv[j] = idum;
	if (iy < 1)
		iy += IMM1;
	temp = AM*iy;
	return temp > RNMX ? RNMX : temp; 
}

