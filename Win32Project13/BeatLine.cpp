#include "BeatLine.h"

// �������� ����
string BeatLine::getLineType()
{
	return lineType;
}

// ������� ����
// string str - ���� 
void BeatLine::Print(string *str)
{
	str[0] = "|";
	str[1] = "";
}