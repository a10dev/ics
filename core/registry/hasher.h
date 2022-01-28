

#pragma once
#include "parser.h"

namespace ics
{
	namespace registry
	{
		class Hasher
		{
		public:

			// Задача этого метода, посчитать хэш по уже построенному списку ключей (подключей),
			// некоторого конкретного фрагмента реестра.
			// В случае успеха, результат возвращается в _outHash аргументе.
			virtual std::string calculateKey(const KeyValues& keyValuesList);

			// Считает хеш бинарной последовательности, в случае успеха возвращает строку с вычисленным хешом.
			virtual std::string getHash(const char* _pData, const unsigned long  _sizeOfData) = 0;

			virtual ~Hasher(){}
		};
	}
}
