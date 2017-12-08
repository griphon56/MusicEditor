#include "Note.h"
#include "Fragment.h"

// �������� �������� ����
char Note::getName()
{
	return Name;
}

// �������� ���������� (���� ������ �����)
char Note::getAlterative()
{
	return Alterative;
}

// �������� ������
int Note::getOctave()
{
	return Octave;
}

// ���������� ������ 
void Note::setOctave(int oct)
{
	Octave = oct;
}

// ���������� ��������
void Note::setName(char name)
{
	Name = name;
}

// ���������� ����������
void Note::setAlterative(char alt)
{
	Alterative = alt;
}

// ������� ����
void Note::Print(string *str)
{
	int type = atoi(&str[2][0]), number = atoi(&str[3][0]);
	unsigned j;
	string fullName = "  ";
	fullName[0] = Name;
	fullName[1] = Alterative;
	for (j = 0; j < 12; ++j)
	{
		if (allNotes[type][j] == fullName)
			break;
	}

	str[0] = "  ";
	str[0][0] = Name;

	if ((allNotes[type][j][1] == '#' && getSharpOrder(allNotes[type][j][0]) <= number) || (allNotes[type][j][1] == 'b' && getSharpOrder(allNotes[type][j][0]) >= number))
		str[0][1] = ' ';
	else str[0][1] = Alterative;
	str[0] += " (" + octaveNames[Octave] + ")";
	str[1] = " 1/" + to_string(getDuration());

}

// �������� ���� ����������
// char noteLetter - �������� ����
int Note::getSharpOrder(char noteLetter)
{
	map<char, int> sharpOrder;
	sharpOrder['F'] = 1; sharpOrder['C'] = 2; sharpOrder['G'] = 3; sharpOrder['D'] = 4; sharpOrder['A'] = 5; sharpOrder['E'] = 6; sharpOrder['B'] = 7;
	return sharpOrder[noteLetter];
}

// �������� ���������� ����
// int type - ��� ����������
// int number - ����� ���� 
void Note::getFullName(int type, int number)
{
	if (getAlterative() == ' ')
	{
		if (type == 0 && getSharpOrder(getName()) <= number)
			Alterative = '#';
		else if (type == 1 && getSharpOrder(getName()) >= number)
			Alterative = 'b';
	}
}

// ���������������� ����� ��� ���� �� ������.
// int interval - �������� ��������.
// int type - ��� ����������.
// int number - ����� ����.
void Note::transpose(int interval, int type, int number)
{
	string noteName = " ";
	noteName[0] = Name;
	noteName += Alterative;

	unsigned j;
	for (j = 0; j < 12; ++j)
	{
		if (allNotes[type][j] == noteName)
			break;
	}

	int newj = j + interval;
	j = (newj + 12) % 12;
	unsigned oct = getOctave() + newj / 12;
	if (oct < 0)
		setOctave(oct);
	else
		setOctave(getOctave() + newj / 12);

	setName(allNotes[type][j][0]);
	setAlterative(allNotes[type][j][1]);
}

// ����� ����������, � �������� ��������� ���������� ������.
Note Note::operator + (int interval)
{
	Note *note = new Note(*this);

	string noteName = " ";
	noteName[0] = Name;
	noteName += Alterative;

	unsigned j, i;
	for (i = 0; i < 2; ++i)
	{
		for (j = 0; j < 12; ++j)
		{
			if (allNotes[i][j] == noteName)
			{
				int newj = j + interval;
				j = (newj + 12) % 12;
				unsigned oct = getOctave() + newj / 12;
				if (oct < 0)
					note->setOctave(oct);
				else
					note->setOctave(getOctave() + newj / 12);

				note->setName(allNotes[i][j][0]);
				note->setAlterative(allNotes[i][j][1]);

				return *note;
			}
		}
	}

}