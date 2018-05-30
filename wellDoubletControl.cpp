#ifndef WELLDOUBLETCONTROL_CPP
#define WELLDOUBLETCONTROL_CPP

#define LOG(x) std::cout << x << std::endl 
#include <math.h>
#include <iostream>
#include "comparison.h"

class WellSchemeA;
class WellSchemeB;
class WellSchemeC;

struct WellDoubletCalculation
{
	double Q_H, Q_w;  // power rate, flow rate
	double T1, T2;  // temperature at well 1 ("warm well") and 2 ("cold well")
	
	bool flag_powerrateAdapted;

	void adapt_powerrate(WellSchemeA* scheme);
	void adapt_powerrate(WellSchemeB* scheme);
	void adapt_powerrate(WellSchemeC* scheme);

	void calculate_flowrate(WellSchemeA* scheme);
	void calculate_flowrate(WellSchemeB* scheme);
	void calculate_flowrate(WellSchemeC* scheme);
};


struct WellDoubletControl
{
	double epsilon;  // accuracy
	WellDoubletCalculation calculation;
	double value_target, value_threshold;
	// A: T1_target, Q_w_max 
	// B: Q_w_target, T1_max 
	// C: DT_target, T1_max

	double capacity;  // set in iteration loop later on

	Comparison beyondThreshold, check_for_flow_rate_adaption;
public:
	WellDoubletControl() : epsilon(1.e-5), capacity(5e6) {}
	virtual ~WellDoubletControl() {}
	virtual void initialize() = 0;

	void set_timeStepValues(const double& _Q_H,
		const double& _value_target, const double& _value_threshold) 
	{ 
		LOG("set time step values");
		calculation.Q_H = _Q_H;
		value_target = _value_target;
		value_threshold = _value_threshold;

		calculation.flag_powerrateAdapted = false;
		initialize();
	}

	void set_iterationValues(const double& _T1, const double& _T2)  
	{
		LOG("set iteration values");
		calculation.T1 = _T1;
		calculation.T2 = _T2;
	}

	virtual void calculate_flowrate() = 0;

	virtual bool check_result() = 0;	
	WellDoubletCalculation& get_result()	{ return calculation; }

	static WellDoubletControl* createWellDoubletControl(
						const char& selection);
	
};

class WellSchemeA : public WellDoubletControl
{
public:
	void initialize()
	{
		LOG("initialize");
		//calculation.T1 = value_target;

		if(calculation.Q_H > 0.)
		{
			beyondThreshold = Comparison(new Greater(1.e-5));
			check_for_flow_rate_adaption =  Comparison(new Smaller(1.e-5));
		}
		else
		{
			beyondThreshold = Comparison(new Smaller(1.e-5));
			check_for_flow_rate_adaption = Comparison(new Greater(1.e-5));
		}
	}

	void calculate_flowrate() { calculation.calculate_flowrate(this); }

	bool check_result()
	{
		LOG("check result");
		if(beyondThreshold(calculation.T1, value_target))
		{
			calculation.adapt_powerrate(this);
			return true;
		}
		else
		{
			if(check_for_flow_rate_adaption(calculation.T1, value_target))
			{
				calculation.calculate_flowrate(this);
				return true;
			}
		}
		return false;
	}
};

class WellSchemeB : public WellDoubletControl
{

public:
	void initialize()
	{
		if(calculation.Q_H > 0.)  // storing
			beyondThreshold = Comparison(new Greater(0.));
		else  // extracting
			beyondThreshold = Comparison(new Smaller(0.));
	}

	void calculate_flowrate() { calculation.calculate_flowrate(this); }

	bool check_result()
	{
		if(beyondThreshold(calculation.T1, value_threshold))
		{
			calculation.adapt_powerrate(this);
			return true;
		}
		return false;
	}
};


class WellSchemeC : public WellDoubletControl
{

public:
	void initialize()
	{
		if(calculation.Q_H > 0.)
		{
			beyondThreshold = Comparison(new Greater(0.));
		}
		else
		{
			beyondThreshold = Comparison(new Smaller(0.));
		}
	}

	void calculate_flowrate() { calculation.calculate_flowrate(this); }

	bool check_result()
	{
		if(beyondThreshold(calculation.T1, value_threshold))
		{
			calculation.adapt_powerrate(this);
			return true;
		}
		return false;
	}
};


WellDoubletControl* WellDoubletControl::createWellDoubletControl(
						const char& selection)
{
	switch(selection)
	{
		case  'A':
			return new WellSchemeA;
			break;
		case  'B':
			return new WellSchemeB;
			break;
		case  'C':
			return new WellSchemeC;
			break;
		default:
			break;
	}
	return 0;
}


void WellDoubletCalculation::adapt_powerrate(WellSchemeA* scheme)
{
	LOG("adapt power rate");
	Q_H = Q_w * scheme->capacity * (2 * scheme->value_target - T1 - T2);
	flag_powerrateAdapted = true;

}

void WellDoubletCalculation::adapt_powerrate(WellSchemeB* scheme)
{
	LOG("adapt power rate");
	Q_H = Q_w * scheme->capacity * (scheme->value_threshold - T2);
	flag_powerrateAdapted = true;
}

void WellDoubletCalculation::adapt_powerrate(WellSchemeC* scheme)
{
	LOG("adapt power rate");
	Q_H = Q_w * scheme->capacity * (scheme->value_threshold - T2);
	flag_powerrateAdapted = true;

}


void WellDoubletCalculation::calculate_flowrate(WellSchemeA* scheme)
{
	if(Q_H > 0)
		Q_w = fmin(Q_H / (scheme->capacity  * (scheme->value_target - T2)), scheme->value_threshold);
	else
		Q_w = fmax(Q_H / (scheme->capacity  * (scheme->value_target - T2)), scheme->value_threshold);
}

void WellDoubletCalculation::calculate_flowrate(WellSchemeB* scheme)
{
	Q_w = scheme->value_target;
}

void WellDoubletCalculation::calculate_flowrate(WellSchemeC* scheme)
{
	// no range check
	if(Q_H > 0)
		Q_w = Q_H / (scheme->capacity *  scheme->value_target);
	else
		Q_w = Q_H / (scheme->capacity *  scheme->value_target);
}

#endif