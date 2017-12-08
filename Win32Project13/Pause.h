#pragma once
#include "resource.h"
#include "Element.h"

class Pause : public Element
{
public:
	Pause(int dur = 1) { Type = 2; Duration = dur; }
	void Print(string *str);
};
