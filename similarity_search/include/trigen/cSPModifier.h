/*
 * Part of TriGen (Tomas Skopal, Jiry Novak)
 * Downloaded from http://siret.ms.mff.cuni.cz/skopal/download.htm
 */
#pragma once

#include <cmath>
#include <string>
#include <cstdio>

using namespace std;

class cSPModifier
{
protected:
	double	  m_ConcavityWeight;
public:
	cSPModifier(void)
	{
		m_ConcavityWeight = 0;		
	}

	virtual ~cSPModifier(void) {};
	
	virtual void SetConcavityWeight(double cw)
	{
		m_ConcavityWeight = cw;
	}

	double GetConcavityWeight()
	{
		return m_ConcavityWeight;
	}

	virtual double ComputeModification(double value) = 0;

	virtual string GetInfo() = 0;
};

//****************************************************************************

class cFractionalPowerModifier : public cSPModifier
{
public:

	cFractionalPowerModifier(double cw)
	{
		m_ConcavityWeight = cw;
	}

	virtual double ComputeModification(double value)
	{
		return pow(value, 1.0 / (1.0 + m_ConcavityWeight));
	}

	virtual string GetInfo() override { return "Fractional Power Modifier"; }
};
