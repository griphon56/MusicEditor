#include "Chrd.h"

// Добавить ноту в аккорд.
// obj element - нота.
void Chrd::addNote(Note *element)
{
	Note * newelement = new Note(*element);
	Notes.push_back(newelement);
}

// Вывести аккорд
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

// Получить ноты аккорда.
vector<Note *>Chrd::getNotes()
{
	return Notes;
}