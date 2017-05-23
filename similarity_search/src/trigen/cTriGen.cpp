#include <limits>

#include "trigen/cTriGen.h"
#include "trigen/cUniformRandomGenerator.h"

//#include <float.h>
#include "logging.h"

const double DBL_MAX = numeric_limits<double>::max();

cTriGen::cTriGen(const cSpaceProxy& distance, const ObjectVector &source, unsigned int sampleSize, const vector<cSPModifier*> &modifBases) : mDistance(distance), mModifierBases(modifBases)
{
	unsigned int len = sampleSize * sampleSize;

	mDistanceMatrix = new double [len];
	for(unsigned int i=0; i<len; i++)
		mDistanceMatrix[i]=-1;

	mItems.resize(sampleSize);
	mCount = sampleSize;

	SampleItems(source);
}

cTriGen::~cTriGen(void)
{	
	delete [] mDistanceMatrix;	
}

int cTriGen::compareOrderIndexPairs(const void* item1, const void* item2)
{
	return ((order_index_pair*)item1)->order < ((order_index_pair*)item2)->order;
}

void cTriGen::SampleItems(const ObjectVector &source)
{
	cUniformRandomGenerator gen(17);

	order_index_pair* reordering_array = new order_index_pair [source.size()];

	for(unsigned int i = 0; i < source.size(); i++)
	{
    // Leo's comment, order can be large or negative but this is fine b/c this value 
    // is used for comparison/sorting purposes only (i.e, it is never used as 
    // an index)
		reordering_array[i].order = (int)(gen.GetNext() * source.size());
		reordering_array[i].index = i;
	}

	qsort((void*)reordering_array, source.size(), sizeof(order_index_pair), compareOrderIndexPairs);

  CHECK(mItems.size() == mCount);
	for(unsigned int i = 0; i < mCount; i++)
	{
    int reordIndex = reordering_array[i].index;
    CHECK_MSG(reordIndex >= 0 && reordIndex < source.size(), 
              "Unexpected index: " + ConvertToString(reordIndex)); 
		mItems[i] = source[reordIndex];
	}

	delete [] reordering_array;
}

double cTriGen::ComputeTriangleError(unsigned int tripletSampleCount, double errorToleranceSkip, eSamplingTriplets samplingTriplets)
{		
	cUniformRandomGenerator gen(13);
	unsigned int a,b,c;
	cOrderedTriplet triplet;

	unsigned int nonTriangularCount;
	nonTriangularCount = 0;

	unsigned int errorThreshold = (int)((double)tripletSampleCount*errorToleranceSkip);

	for(unsigned int i = 0; i < tripletSampleCount ; )
	{	
		a = (unsigned int)((mCount - 1) * gen.GetNext());

		//if (samplingTriplets == eRandom)
		{			
			b = (unsigned int)((mCount - 1) * gen.GetNext()); 
			c = (unsigned int)((mCount - 1) * gen.GetNext());
		} 

		triplet.SetTriplet(GetModifiedDistance(a,b), GetModifiedDistance(b,c), GetModifiedDistance(a,c));
		if (triplet.isRegular())
		{
			i++;
			if (!triplet.isTriangular())
			{
				nonTriangularCount++;
				if (nonTriangularCount > errorThreshold)
					return (double)nonTriangularCount/tripletSampleCount;
			}
		}
	}
	
	return (double)nonTriangularCount/tripletSampleCount;
}

void cTriGen::ComputeDistribution(unsigned int distanceSampleCount, double& mean, double& variance, double& idim)
{
	mean = variance = 0;
	double dist;

	cUniformRandomGenerator gen1(13), gen2(13);

	for(unsigned int i=0; i<distanceSampleCount; i++)
	{		
    unsigned id1 = (unsigned int)((mCount-1) * gen1.GetNext());
    unsigned id2 = (unsigned int)((mCount-1) * gen1.GetNext());

		dist = GetModifiedDistance(id1, id2);
    CHECK_MSG(!isnan(dist), "The modified distance is mistakingly NAN!");
		mean += dist;
	}		
	mean /= distanceSampleCount;

	for(unsigned int i=0; i<distanceSampleCount; i++)
	{		
		dist = GetModifiedDistance((unsigned int)((mCount-1) * gen2.GetNext()),  (unsigned int)((mCount-1) * gen2.GetNext()));
		variance += __SQR(dist - mean);
	}		
	variance /= distanceSampleCount;

	idim = __SQR(mean)/(2*variance);	
}

cSPModifier* cTriGen::Run(/* in/out */double& errorTolerance, /* out */unsigned int& funcOrder, /* out */ double& resultIDim, /* in */ unsigned int tripletSampleCount,  /* in */ bool echoOn, /* in */ eSamplingTriplets samplingTriplets)
{
	unsigned iterLimit = 24;
	double w_LB, w_UB, w_best, error, err_best = DBL_MAX, inputTolerance = errorTolerance;
	cSPModifier* result = NULL;

	double mean, variance, idim;
	resultIDim = DBL_MAX;
	//char infoBuffer[256];

	if (echoOn)
		LOG(LIB_INFO) << "TriGen started, " <<  mModifierBases.size() << " bases are going to be tried, " << tripletSampleCount << " sampled triplets"; 

	for(unsigned int i=0; i<mModifierBases.size(); i++)
	{
		w_LB = 0; w_UB = DBL_MAX; 		
		w_best = -1;

		mCurrentModifier = mModifierBases[i];					
		mCurrentModifier->SetConcavityWeight(1);
		if (echoOn)
		  LOG(LIB_INFO) << " Trying modifier: " << mCurrentModifier->GetInfo();

		for(unsigned int iter = 0; iter < iterLimit; iter++)
		{
			ClearModifiedDistances();			

			error = ComputeTriangleError(tripletSampleCount, inputTolerance, samplingTriplets);
			if (error <= inputTolerance)
			{
				w_UB = w_best = mCurrentModifier->GetConcavityWeight();
				err_best = error;
				mCurrentModifier->SetConcavityWeight((w_LB + w_UB)/2);				
			}
			else
			{
				w_LB = mCurrentModifier->GetConcavityWeight();
				if (w_UB != DBL_MAX)
					mCurrentModifier->SetConcavityWeight((w_LB + w_UB)/2);				
				else
					mCurrentModifier->SetConcavityWeight(2.0*mCurrentModifier->GetConcavityWeight());
			}
		}

		if (w_best > -1)
		{
			ClearModifiedDistances();
			mCurrentModifier->SetConcavityWeight(w_best);			
			ComputeDistribution(tripletSampleCount, mean, variance, idim);	
			
			if (idim < resultIDim)
			{
				funcOrder = i; result = mCurrentModifier; 
				resultIDim = idim;
				errorTolerance = err_best;
				if (echoOn)				
					LOG(LIB_INFO) << "found the best so far - idim: " << idim << " cw:  " << mCurrentModifier->GetConcavityWeight() << ", err: " <<  err_best;
			} else
			{
				if (echoOn)
				  LOG(LIB_INFO) << "found but not the best (idim: " << idim << ")";
			}

		} else
		{
			if (echoOn)
			  LOG(LIB_INFO) << "not found";
		}

	}
	return result;
}

