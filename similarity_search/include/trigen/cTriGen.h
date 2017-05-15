#ifndef __cTriGen_h__
#define __cTriGen_h__

#include "trigen/utils.h"
#include "trigen/cDistance.h"
#include "trigen/cArray.h"
#include "trigen/cSPModifier.h"

enum eSamplingTriplets
{
	eRandom = 0,
	eDivergent
};

class cOrderedTriplet
{
	double mTriplet[3];

	void swap(double& a, double& b)
	{
		double c=a;
		a=b; b=c;
	}

public:	
	void SetTriplet(double xx, double yy, double zz)
	{
		mTriplet[0] = xx; mTriplet[1] = yy; mTriplet[2] = zz;
		if (mTriplet[2] < mTriplet[1])
			swap(mTriplet[2], mTriplet[1]);
		if (mTriplet[1] < mTriplet[0])
			swap(mTriplet[1], mTriplet[0]);
		if (mTriplet[2] < mTriplet[1])
			swap(mTriplet[2], mTriplet[1]);
	}

	void Modify(typeSPModifier* func)
	{
		mTriplet[0] = func(mTriplet[0]);
		mTriplet[1] = func(mTriplet[1]);
		mTriplet[2] = func(mTriplet[2]);
	}

	bool isTriangular()
	{
		return (mTriplet[0] + mTriplet[1] >= mTriplet[2]);
	}

	bool isRegular()
	{
		return mTriplet[0] > 0;	// since the triplet is ordered, it is sufficient to test the first member
	}
};

class cTriGen
{
	double			*mDistanceMatrix;
	cDataObject		**mItems;
	cSPModifier		*mCurrentModifier;
	cArray<cSPModifier*> mModifierBases;
	cDistance		*mDistance;

	unsigned int mCount;

	struct order_index_pair {int order; int index;};

	static int compareOrderIndexPairs(const void* item1, const void* item2);

	double GetDistance(unsigned int x, unsigned int y)
	{
		if (x > y)
		{
			unsigned int c = x;
			x = y; y = c;
		}

		double& val = mDistanceMatrix[mCount * y + x];
		if (val == -1)
		{
			val = mDistance->Compute(*mItems[x], *mItems[y]);
			mDistanceMatrix[mCount * y + x] = val;
		}

		return val;
	}

	double GetModifiedDistance(unsigned int x, unsigned int y)
	{
		if (x < y)
		{
			unsigned int c = x;
			x = y; y = c;
		}

		double& val = mDistanceMatrix[mCount * y + x];
		if (val == -1)
		{
			val = mCurrentModifier->ComputeModification(GetDistance(x,y));
			mDistanceMatrix[mCount * y + x] = val;
		}

		return val;
	}

	void ClearCellModified(unsigned int x, unsigned int y)
	{
		if (x < y)
		{
			unsigned int c = x;
			x = y; y = c;
		}

		mDistanceMatrix[mCount * y + x] = -1;
	}

	void ClearModifiedDistances()
	{
		for(unsigned int y = 0; y < mCount; y++) 
			for(unsigned int x = y + 1; x < mCount; x++)
				ClearCellModified(x,y);
	}

	void SampleItems(cArray<cDataObject> &source);
	double ComputeTriangleError(unsigned int tripletSampleCount, double errorToleranceSkip = 1 /* 1 = do not use skipping*/, eSamplingTriplets samplingTriplets = eRandom);	

	void ComputeDistribution(unsigned int distanceSampleCount, double& mean, double& variance, double& idim);

public:
	cTriGen(cDistance* distance, cArray<cDataObject> &source, unsigned int sampleSize, cArray<cSPModifier*> &modifBases);
	~cTriGen(void);

	cSPModifier* Run(double& errorTolerance, unsigned int& funcOrder, double& resultIDim, unsigned int tripletSampleCount, bool echoOn = true, eSamplingTriplets samplingTriplets = eRandom);

	void GetSampledItems(cArray<cDataObject>& output)
	{
		for(unsigned int i=0; i<mCount; i++)
			output.Add(mItems[i]);
	}
};

#endif
 
