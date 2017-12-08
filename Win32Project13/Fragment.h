#pragma once
#include "resource.h"
#include "Element.h"
#include "Note.h"

const string allNotes[2][12] = { {"C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B "}, {"C ", "Db", "D ", "Eb", "E ", "F ", "Gb", "G ", "Ab", "A ", "Bb", "B "}};
const string octaveNames [7] = {"Contra", "Great", "Low", "First", "Second", "Third", "Fourth"};

class Fragment
{
	struct{
		int Type;
		int Number;
	}  Alterative;

	struct {
		int Numerator, Denominator;
	} Form;

	vector<Element *> Elements;

public:
	Fragment(int num = 4, int den = 4, int type = 0, int number = 0);
	void addElement(Element *);
	void transpose(int interval);
	void setBeatLines();
	void printFragment(HWND &wind, HINSTANCE &hInst);
	int getInterval(Note *note1, Note* note2);
	string getTonality();
	void printTact(HWND &wind, HINSTANCE &hInst, int number);
	int getLength();
	int getType();
	int getNumber();

	void readFragment(string filename);
	Element * findElement(int x, int y);
};