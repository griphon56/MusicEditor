#include "Pause.h"

// Вывести паузу
void Pause::Print(string *str)
{
	str[0] = "Pause";
	str[1] = "1/" + to_string(Duration);
}