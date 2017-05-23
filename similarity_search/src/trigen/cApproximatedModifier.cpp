#include "trigen/cApproximatedModifier.h"

#include "logging.h"

const double REAL_TOLERANCE = 0.000001;
const double SHIFT_TOLERANCE = 0.0001;

#define ABS(x) (((x)<0) ? -(x) : (x))

bool cApproximatedModifier::TestFloatEqual(double x, double y)
{
	if (y >= x - REAL_TOLERANCE && y <= x + REAL_TOLERANCE)
		return true;
	else
		return false;
}


bool cApproximatedModifier::VerifyMonotonocity()
{
	// verify increasing monotonicity
	for(unsigned int i = 0; i < mStepCount; i++)
	{
		if (mValues[i] - SHIFT_TOLERANCE > mValues[i+1])
			return false;
	}

	return true;
}

bool cApproximatedModifier::VerifyConcavity()
{	
	// verify concavity

	for(unsigned int i = 0; i < mStepCount - 1; i++)
	{		
		if ( (mValues[i] + mValues[i+2]) - 2*SHIFT_TOLERANCE > 2.0 * mValues[i+1])
			return false;
	}

	return true;
}

void cApproximatedModifier::MakeLinearApproximation(unsigned int stepCount)
{
	if (stepCount > mStepCount)
	{
		if (mStepCount !=0 )
			delete [] mValues;

		mStepCount = stepCount;
		mValues = new double [mStepCount+1];
	}

	for(unsigned int i=0; i<=mStepCount; i++)
	{
		mValues[i] = this->ComputeNonApproximatedValue((double)i/(double)(mStepCount));
		
		/*if (i>0 && mValues[i] < mValues[i-1])
			mValues[i] = mValues[i-1] + ADDITIVE_TOLERANCE;*/
	}
	m_ApproximationValid = true;
}

double cApproximatedModifier::ComputeModification(double x)
{
	if (!m_ApproximationValid)
		MakeLinearApproximation(mStepCount);

	CHECK(mStepCount != 0);

	if (TestFloatEqual(x, 1.0))
		return 1;

	unsigned int loweri = (unsigned int)(x*(double)mStepCount);
	unsigned int upperi = loweri+1;

	double result = mValues[loweri] + (mValues[upperi] - mValues[loweri])*(x-(double)loweri/(double)mStepCount)/((double)upperi/(double)mStepCount-(double)loweri/(double)mStepCount);

	return result;
}

