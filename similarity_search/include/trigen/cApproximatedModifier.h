#pragma once

#include "trigen/cSPModifier.h"

const double ADDITIVE_TOLERANCE = 0.00001;


class cApproximatedModifier : public cSPModifier
{
protected:
	double* mValues;
	unsigned int mStepCount;
	bool m_ApproximationValid;	

	void MakeLinearApproximation(unsigned int stepCount);
	virtual double ComputeNonApproximatedValue(double x) = 0;

public:

	cApproximatedModifier(unsigned int stepCount)
	{
		mValues	= NULL;
		mStepCount = stepCount;	
		m_ApproximationValid = false;

		mValues = new double [mStepCount+1];
	}

	virtual ~cApproximatedModifier() override
	{
		if (mValues	!= NULL)
			delete [] mValues;		
	}

	virtual void SetConcavityWeight(double cw)
	{
		cSPModifier::SetConcavityWeight(cw);
		m_ApproximationValid = false;
	}

	bool VerifyMonotonocity();
	bool VerifyConcavity();

	virtual double ComputeModification(double value);

	static bool TestFloatEqual(double x, double y); 
};
