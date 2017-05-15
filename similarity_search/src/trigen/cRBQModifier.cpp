#include "trigen/cRBQModifier.h"
#include "trigen/utils.h"
#include "logging.h"

double cApproximatedRBQModifier::ComputeNonApproximatedValue(double x)
{
	return cRBQModifier::RBQ(x, m_a, m_b, cRBQModifier::m_ConcavityWeight);
}

double cRBQModifier::RBQ(double x, double a, double b, double w)
{
	double nominator1, nominator2;
	double denominator;

	// handle denominator and square root singularities						
	if (x == 0)
		return 0.0;
	else if (x == 1)
		return 1.0;			
	
	if (cApproximatedModifier::TestFloatEqual(a, 0.5))
		a += ADDITIVE_TOLERANCE;
	
	if (w != 1.0 && cApproximatedModifier::TestFloatEqual(x, (2.0*(w*a) - 1.0)/(2.0*(w - 1.0))))		
		x += ADDITIVE_TOLERANCE;		
		//w += ADDITIVE_TOLERANCE;		

	while (cApproximatedModifier::TestFloatEqual(w, 1.0/(2*sqrt(a-__SQR(a)))))
		a += ADDITIVE_TOLERANCE;

	double square_root_base = __SQR(x)*(__SQR(w)-1.0) + __SQR(w)*a*(a-2*x)+x;

	CHECK(square_root_base >= 0);

	double square_root = sqrt(square_root_base);
	nominator1 = x + w*(a - x) - square_root;
	nominator2 = 2*w*b*(w*(x - a) - x + 1.0) + square_root*(1.0 - 2.0*w*b) - x + w*(x - a);
	denominator = -1.0 + 2*w*a*(1.0 - 2*x + w*(1.0 - 2*a)) + 2*square_root*(1.0 - w) + 2*x*w*(1.0 - w + 2*w*a);

	CHECK(denominator != 0);

	// numeric optimization
	double result;
	if (abs(nominator2) < abs(nominator1))
		result = (nominator2/denominator) * nominator1;
	else
		result = (nominator1/denominator) * nominator2;

	result = __MIN(result, 1);
	result = __MAX(result, 0);

	return result;
}
