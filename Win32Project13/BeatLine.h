#pragma once
#include "resource.h"
#include "Element.h"

class BeatLine : public Element
{
	string lineType; //{boffStart, boffEnd, tactEnd, fragmentEnd};

public:
	BeatLine(string type = "tactEnd") : lineType(type) { Type = 3; Duration = 0; }
	string getLineType();
	void Print(string *str);
};