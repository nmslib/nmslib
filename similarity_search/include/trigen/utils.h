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
/*
inline char* FIN(int number, char* buffer, int strLen)
{
	buffer[0] = 0;	
	sprintf_s(buffer, 32, "%i", number);
	memset(buffer + strlen(buffer), ' ', strLen - strlen(buffer));
	buffer[strLen]=0;	
	return buffer;
};

inline char* FDN(double number, char* buffer, int strLen)
{
	sprintf_s(buffer, 32, "%.4f", number);
	int len = (int)strlen(buffer);
	memset(buffer, ' ', strLen-len);
	sprintf_s(buffer + strLen - len, 32, "%.4f", number);
	buffer[strLen] = 0;	
	return buffer;
};

inline void MyPrint(FILE* f, const char* formatString, ...)
{
	va_list marker;

	va_start(marker, formatString);

	if (f == NULL)
	{
		vprintf(formatString, marker);
	} 
	else
	{
		vfprintf(f, formatString, marker);
	}
};
*/

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
