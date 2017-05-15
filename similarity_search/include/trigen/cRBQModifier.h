#pragma once

#include "trigen/cApproximatedModifier.h"

#include <sstream>

using namespace std;

class cRBQModifier : public cSPModifier
{
	protected:		
	double m_a, m_b;

	static double RBQ(double x, double a, double b, double w);

public:

	cRBQModifier(double a, double b)
	{
		m_a = a; m_b = b;
	}

	cRBQModifier(double a, double b, double concavity)
	{
		m_a = a; m_b = b; m_ConcavityWeight = concavity;
	}


	double GetA()
	{
		return m_a;
	}

	double GetB()
	{
		return m_b;
	}

	virtual double ComputeModification(double value)
	{
		return RBQ(value, m_a, m_b, m_ConcavityWeight);
	}

	virtual string GetInfo() override
	{		
    stringstream ss;
		ss << "RBQ Modifier (a = " << m_a <<", b = " << m_b <<")";
		return ss.str();
	}
};

class cApproximatedRBQModifier : public cApproximatedModifier, cRBQModifier
{	
public:

	cApproximatedRBQModifier(double a, double b, unsigned int stepCount) : cApproximatedModifier(stepCount), cRBQModifier(a,b)
	{
		
	}

	virtual double ComputeNonApproximatedValue(double x);	

	virtual string GetInfo() override
	{		
    stringstream ss;
		ss << "Approximated RBQ Modifier (a = " << m_a <<", b = " << m_b <<")";
		return ss.str();
	}
};
