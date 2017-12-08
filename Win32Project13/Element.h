#pragma once
#include "resource.h"

class Element
{
protected:
	int Type;
	int Duration;
public:
	int getType();

	int getDuration();
	virtual void Print(string *str) = 0;
};
