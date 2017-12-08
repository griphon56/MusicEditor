#pragma once
#include "resource.h"
#include "Element.h"

class Note : public Element
{
	char Name; //{C, D, E, F, G, A, B};
	char Alterative; //{lack, flat, sharp, bekar};
	int Octave; //{contra - 0, great - 1, low - 2, first - 3, second - 4, third - 5, fourth - 6};

public:
	Note(char name = 'C', char alter = ' ', int oct = 3, int duration = 1) :
		Name(name),
		Alterative(alter),
		Octave(oct)
	{
		Type = 1; Duration = duration;
	}
	char getName();

	int getSharpOrder(char noteLetter);
	void getFullName(int type, int number);
	char getAlterative();
	int getOctave();
	void setOctave(int oct);
	void setName(char name);
	void setAlterative(char alter);
	void Print(string *str);
	void transpose(int interval, int type, int number);

	Note operator + (int interval);
};