/**************************************************************************}
{                                                                          }
{    cMetric.h                                                             }
{                                                                          }
{                                                                          }
{                 Copyright (c) 2002 - 2006          Tomas Skopal          }
{                                                                          }
{    VERSION: 0.9                            DATE 09/04/2006               }
{                                                                          }
{             following functionality:                                     }
{               distance functions		                                   }
{                                                                          }
{    UPDATE HISTORY:                                                       }
{                                                                          }
{                                                                          }
{**************************************************************************/

#ifndef __cDistance_h__
#define __cDistance_h__

#include "trigen/utils.h"
//#include "cStream.h"

enum ePivotSamplingMethod
{
	eRandomGroupMaxDistance = 0,
	eHFAlgorithm
};

class cDataObject
{
protected:
	uint	mId;		// data object id
	byte*	mData;		// unstructured data object representation
	uint	mDataSize;	// used size in bytes
	uint	mAllocSize;	// allocated size

public:
	cDataObject();
	~cDataObject();

	void SetId(uint id)								{ mId = id; }
	void Deattach()									{ mDataSize = 0; mAllocSize = 0; mData = NULL; }
	uint GetId()									{ return mId; }
	byte* GetData()									{ return mData; }

	uint		GetSerialSize() const				{ return mDataSize + sizeof(mId); } 
	static uint	GetSerialSize(uint dataBytes)		{ return dataBytes + sizeof(uint); } 
	uint		GetDataSize() const					{ return mDataSize; } 

	void	FreeData();
	void	ResizeToSerialBytes(uint serialBytes);
	void	ResizeToDataBytes(uint dataBytes);

	int		operator = (const cDataObject &obj);

/*
	bool	ReadSerial(cStream*);		// assumes allocated and defined mDataSize
	bool	WriteSerial(cStream*);		// mDataSize is not written
*/

	void	Attach(cDataObject& obj);	// needed just for M-tree kNN search
	bool	DataEqual(cDataObject& obj);
};

class cDistance 
{
public:
	virtual double Compute(cDataObject& object1, cDataObject& object2) = 0;
	virtual cDistance* Clone() = 0;

};

class cPartialDistance : public cDistance
{	
protected:
	uint mDataOffset;
	uint mDataLength;
public:

	void SetDataOffset(uint offset)		{ mDataOffset = offset; }
	void SetDataLength(uint length)		{ mDataLength = length; }

	virtual cDistance*			Clone() = 0;
	//virtual cPartialDistance*	Clone() = 0;
};

class cPartialFloatVectorDistance : public cPartialDistance
{	
protected:
	virtual double Compute(cDataObject& object1, cDataObject& object2)  { return -1; }

public:
	void SetPartialDimension(uint dim)			{ mDataLength = sizeof(float) * dim; }
	void SetStartCoordinate(uint start)			{ mDataOffset = sizeof(float) * start; }

	uint GetPartialDimension()					{ return mDataLength / sizeof(float); }
	uint GetStartCoordinate()					{ return mDataOffset / sizeof(float); }

	virtual cDistance* Clone()					{ return new cPartialFloatVectorDistance(); }

	
};

class cComplexDistance : public cDistance
{
protected:
	cPartialDistance**	mPartialDistanceArray;
	uint				mPartialDistanceCount;
	float*				mWeights;
public:

	cComplexDistance() { mPartialDistanceArray = NULL; };
	~cComplexDistance();
	cComplexDistance(uint distances, cPartialDistance** distanceArray) { /* not implemented yet */ }
	cComplexDistance(uint distances, cPartialDistance* prototypeDistance, float* weights);
	void Init(uint distances, cPartialDistance* prototypeDistance, float* weights);
	virtual void Destroy();

};

class cComplexFloatVectorDistance : public cComplexDistance
{
public:
	
	cComplexFloatVectorDistance() {};
	cComplexFloatVectorDistance(uint distances, cPartialFloatVectorDistance* prototypeDistance, float* weights, uint* partialDimensions); 
	void Init(uint distances, cPartialFloatVectorDistance* prototypeDistance, float* weights, uint* partialDimensions);

	virtual double Compute(cDataObject& object1, cDataObject& object2);
	virtual cDistance* Clone()	{ return new cComplexFloatVectorDistance(); }

};


#endif
