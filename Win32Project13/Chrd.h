#pragma once
#include "resource.h"
#include "Element.h"
#include "Note.h"

class Chrd : public Element
{
	vector<Note *> Notes;
public:
	Chrd(int dur = 1) { Type = 4; Duration = dur; }
	void addNote(Note *);
	void Print(string *str);
	vector<Note *> getNotes();

};