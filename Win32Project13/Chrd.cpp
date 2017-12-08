#include "Chrd.h"

// �������� ���� � ������.
// obj element - ����.
void Chrd::addNote(Note *element)
{
	Note * newelement = new Note(*element);
	Notes.push_back(newelement);
}

// ������� ������
void Chrd::Print(string *str)
{
	string workstr = "";
	for (unsigned i = 0; i < Notes.size() - 1; ++i)
	{
		Notes[i]->Print(str);
		workstr += str[0] + "\n";
	}
	Notes[Notes.size() - 1]->Print(str);
	workstr += str[0];
	str[0] = workstr;
}

// �������� ���� �������.
vector<Note *>Chrd::getNotes()
{
	return Notes;
}