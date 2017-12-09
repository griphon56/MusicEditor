#include "BeatLine.h"
#include "Chrd.h"
#include "Fragment.h"
#include "Pause.h"

vector <HWND> elems;
vector <HWND> tootips;

// ХЗ
Fragment::Fragment(int num , int den , int type , int number )
{
	Alterative.Type = type;
	Alterative.Number = number;
	Form.Numerator = num;
	Form.Denominator = den;
}

// Добавление элемента
// obj element - Элемент
void Fragment::addElement(Element *element)
{
	if(element->getType() == 1)
	{
		Note *note= reinterpret_cast<Note*>(element);
		note->getFullName(Alterative.Type, Alterative.Number);
	}
	else if(element->getType() == 4)
	{
		Chrd *chrd = reinterpret_cast<Chrd*>(element);
		unsigned ChrdLength = chrd->getNotes().size();
		for(unsigned j = 0; j < ChrdLength; ++j)
			chrd->getNotes()[j]->getFullName(Alterative.Type, Alterative.Number);
	}

	Elements.push_back(element);
}

// Транспонироание
// int interval - интервал который будет перемещен по октаве.
void Fragment::transpose(int interval)
{
	unsigned FragmentLength = Elements.size();
	for(unsigned i = 0; i < FragmentLength; ++i)
	{
		if(Elements[i]->getType() == 1)
		{
			Note *note= reinterpret_cast<Note*>(Elements[i]);
			note->transpose(interval, Alterative.Type, Alterative.Number);

		}
		else if(Elements[i]->getType() == 4)
		{
			Chrd *chrd = reinterpret_cast<Chrd*>(Elements[i]);
			unsigned ChrdLength = chrd->getNotes().size();
			for(unsigned j = 0; j < ChrdLength; ++j)
				chrd->getNotes()[j]->transpose(interval, Alterative.Type, Alterative.Number);
		}
	}
}

// Установить такты
void Fragment::setBeatLines()
{
	unsigned count = 0, fragmentLength = Elements.size();
	for(unsigned i = 0; i <fragmentLength; ++i)
	{
		if(Elements[i]->getType() != 3)
			{
				count += 16 / Elements[i]->getDuration();
				if(count == Form.Numerator * 16 / Form.Denominator)
				{
					Element *newLine = new BeatLine;
					Elements.insert(Elements.begin() + i + 1, newLine);
					++i;
					++fragmentLength;
					count = 0;
				}
			}
		else
		{
			--i;
			--fragmentLength;
			 Elements.erase(Elements.begin() + i, Elements.begin() + i+1);
		}
	}
}

// Вывести фрагмент
void Fragment::printFragment(HWND &wind, HINSTANCE &hInst)
{
	TCHAR buf[1024];
	HWND labelCont;
	
	string znaki[7] = {"F", "C", "G", "D", "A", "E", "B"};
	string alters = ""; 
	string *str = new string[4];

	elems.clear();

	int i;
	if(Alterative.Type == 0)
		for(i = 0; i < Alterative.Number; ++i)
			alters += (znaki[i] + "# ");
	else 
		for(i = Alterative.Number - 1; i > 0; --i)
			alters += (znaki[i] + "b ");
	
	int ID_LABEL = 5001 + elems.size();

	LPWSTR ptr =  toLPWSTR(alters);

	StringCbPrintfW(buf, ARRAYSIZE(buf), L"Знаки альтерации: %s", ptr);
	
	labelCont = CreateWindow(TEXT("Static"), buf, WS_CHILD | WS_VISIBLE, 10, 30, 130 + Alterative.Number * 22, 20, wind, (HMENU)5001, hInst, NULL);
	elems.push_back (labelCont);

	alters = to_string(Form.Numerator) + "/" + to_string(Form.Denominator); 

	ptr =  toLPWSTR(alters);

	StringCbPrintfW(buf, ARRAYSIZE(buf), L"Размер: %s", ptr);

	elems.push_back (CreateWindow(TEXT("Static"), buf, WS_CHILD | WS_VISIBLE, 10, 60, 140, 20, wind, (HMENU)5002, hInst, NULL));


	int xNum =0, yNum = 0;

	unsigned fragmentLength = Elements.size();
	for(unsigned i = 0; i < fragmentLength; ++i)
	{
		str[2] = to_string(Alterative.Type);
		str[3] = to_string(Alterative.Number);

		Elements[i]->Print(str);
		alters = str[0];
		wchar_t wtext[1024];
		mbstowcs(wtext, alters.c_str(), strlen(alters.c_str())+1);
		ptr = wtext;

		labelCont = CreateWindow(TEXT("Static"), ptr, WS_CHILD | WS_VISIBLE |SS_CENTER , 10 + xNum, 90 + yNum, 70, 60, wind, (HMENU)5003 + i, hInst, NULL);
		elems.push_back (labelCont);

		if((i + 1) % 9 == 0)
		{
			xNum = 0;
			yNum += 70;
		}
		else
		{
			xNum += 90;
		}
	}

}

// Получить интервал.
// obj note1 - Первая нота
// obj note2 - Вторая нота
int Fragment::getInterval(Note *note1, Note* note2)
{
	string name1 = " ", name2 = " "; 
	name1[0] = note1->getName();  name2[0] = note2->getName(); 
	name1 += note1->getAlterative();
	name2 += note2->getAlterative();

	unsigned flag = 0;
	int i = 0, j, k;
	while(flag < 2)
	{
		if(allNotes[Alterative.Type][i] == name1)
		{	
			++flag;
			k = i;
		}
		
		if(allNotes[Alterative.Type][i] == name2)
		{	
			++flag;
			j = i;
		}
		++i;
	}
	return (j - k);

}

// Получить тональность
string Fragment::getTonality()
{
	string tonalities[2][8] = {{"C / Am", "G / Em", "D / Bm", "A / F#m", "E / C#m", "B / G#m", "F# / D#m", "C# / A#m"},
								{"C / Am", "F / Dm", "Bb / Gm", "Eb / Dm", "Ab / Fm", "Db / Bbm", "Gb / Ebm", "Cb / Abm"}};

	return tonalities[Alterative.Type][Alterative.Number];
}

// Вывести такт
// int number - Номер такта
void Fragment::printTact(HWND &wind, HINSTANCE &hInst, int number)
{
	unsigned count = 0, itact = 0, fragmentLength = Elements.size();

	if (number != 1)
		for(itact = 0; itact < fragmentLength; ++itact)
		{
			if(Elements[itact]->getType() == 3)
			{
				++count;
				if(count == number) 
					{
						++itact;
						break;
					}
			}
		}

		elems.clear();

		TCHAR buf[1024];
		HWND labelCont;
	
		string znaki[7] = {"F", "C", "G", "D", "A", "E", "B"};
		string alters = ""; 
		string *str = new string[4];

		int i;
		if(Alterative.Type == 0)
			for(i = 0; i < Alterative.Number; ++i)
				alters += (znaki[i] + "# ");
		else 
			for(i = Alterative.Number - 1; i > 0; --i)
				alters += (znaki[i] + "b ");
	
		int ID_LABEL = 5001 + elems.size();

		LPWSTR ptr =  toLPWSTR(alters);

		StringCbPrintfW(buf, ARRAYSIZE(buf), L"Знаки альтерации: %s", ptr);
	
		labelCont = CreateWindow(TEXT("Static"), buf, WS_CHILD | WS_VISIBLE, 10, 30, 130 + Alterative.Number * 22, 20, wind, (HMENU)5001, hInst, NULL);
		elems.push_back (labelCont);

		alters = to_string(Form.Numerator) + "/" + to_string(Form.Denominator); 

		ptr =  toLPWSTR(alters);

		StringCbPrintfW(buf, ARRAYSIZE(buf), L"Размер: %s", ptr);

		elems.push_back (CreateWindow(TEXT("Static"), buf, WS_CHILD | WS_VISIBLE, 10, 60, 140, 20, wind, (HMENU)5002, hInst, NULL));



	if(itact == fragmentLength)
		MessageBox(NULL, L"Такого такта нет!", L"Ошибка", MB_OK);
	else
	{

		int xNum =0, yNum = 0;

		unsigned fragmentLength = Elements.size();

		for(; itact < fragmentLength; ++itact)
		{
			str[2] = to_string(Alterative.Type);
			str[3] = to_string(Alterative.Number);

			Elements[itact]->Print(str);
			alters = str[0];
			wchar_t wtext[1024];
			mbstowcs(wtext, alters.c_str(), strlen(alters.c_str())+1);
			ptr = wtext;

			labelCont = CreateWindow(TEXT("Static"), ptr, WS_CHILD | WS_VISIBLE |SS_CENTER , 10 + xNum, 90 + yNum, 70, 60, wind, (HMENU)5003 + i, hInst, NULL);
			elems.push_back (labelCont);

			if((itact + 1) % 9 == 0)
			{
				xNum = 0;
				yNum += 70;
			}
			else
			{
				xNum += 90;
			}
			if(Elements[itact]->getType() == 3) break;
		}
	}
}

// Открыть файл с нотами
// string filename - название файла
void Fragment::readFragment(string filename)
{
	ifstream fin(filename);
	fin >> Alterative.Type >> Alterative.Number;
	fin >> Form.Numerator  >> Form.Denominator;
	Elements.clear();
	while(!fin.eof())
	{
		int type;
		fin >> type;
		switch (type)
		{
		case 1:
			{
				char name, alt; int oct, dur;
				fin >> name >> alt >> oct >> dur;
				addElement(new Note(name, alt == '_'?' ':alt, oct, dur));
				break;
			}
		case 2:
			{
				int dur;
				fin >> dur;
				addElement(new Pause(dur));
				break;
			}
		case 4:
			{
				int count, dur;
				fin >> count >> dur;
				Chrd *chrd = new Chrd(dur);
				for(int i = 0; i < count; ++i)
				{
					char name, alt; int oct, dur;
					fin >> name >> alt >> oct >> dur;
					chrd->addNote(new Note(name, alt == '_'?' ':alt, oct, dur));
				}
				addElement(chrd);
				break;
			}

		default:
			break;
		}
	}
}

// Добавление ноты во фрагмент
// char name_note - Название ноты
// char alt_note - Альтерация
// int oct_note - Октава
// int  dur_note - Длительность
void Fragment::addNoteFragment(char name_note, char alt_note, int oct_note, int  dur_note)
{
	addElement(new Note(name_note, alt_note == '_' ? ' ' : alt_note, oct_note, dur_note));
}

// Получить длинну элемента
int Fragment::getLength()
{
	return Elements.size();
}

// Поиск элемента по клику
// int x - Координата Х
// int y - Координата Y
Element * Fragment::findElement(int x, int y)
{
	unsigned fragmentLength = Elements.size();
	int xNum = 0, yNum = 0;
	Element * newelem;

	for(unsigned i = 0; i < fragmentLength; ++i)
	{

		if((x >= 10 + xNum) && (x<= 60 + xNum) && (y >= 90 + yNum) && (y <= 150 + yNum))
		{
			newelem = Elements[i];
			return Elements[i];
		}
		if((i + 1) % 9 == 0)
		{
			xNum = 0;
			yNum += 70;
		}
		else
		{
			xNum += 90;
		}
	}

	return nullptr;
}

// Получить вид альтерации
int Fragment::getType()
{
	return Alterative.Type;
}

// Получить номер альтерации
int Fragment::getNumber()
{
	return Alterative.Number;
}