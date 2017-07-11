/*
 * Part of TriGen (Tomas Skopal, Jiry Novak)
 * Downloaded from http://siret.ms.mff.cuni.cz/skopal/download.htm
 */
#ifndef __utils_h__
#define __utils_h__

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdint>

#define __SQR(x) ((x)*(x))
#define __MIN(a,b) (((a) < (b)) ? (a) : (b))
#define __MAX(a,b) (((a) > (b)) ? (a) : (b))
#define __ABS(x) (((x) < 0) ? (-(x)) : (x))

using namespace std;


typedef double typeSPModifier(double distance);

typedef unsigned int uint;
typedef unsigned char byte;

inline double Fact(unsigned int n)
{
	long double result = 1;
	for (unsigned int i=2; i<=n; i++)
		result *= i;
	return result;
}

inline double Combine(unsigned int n, unsigned int i)
{	
	if (i==0) return 1;
	long double result = n/Fact(i);
	for(unsigned int j = n-1; j>(n-i); j--)
		result *= j;

	return result;
}

#endif
