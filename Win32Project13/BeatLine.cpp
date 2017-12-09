#include "BeatLine.h"

// Получить такт
string BeatLine::getLineType()
{
	return lineType;
}

// Вывести такт
// string str - такт 
void BeatLine::Print(string *str)
{
	str[0] = "|";
	str[1] = "";
}