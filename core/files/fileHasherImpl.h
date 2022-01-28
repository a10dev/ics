

#pragma once

#include <string>

namespace ics
{
	namespace files
	{
		enum HasherErrors
		{
			NoError, // Means success.
			Missed,
			LockedAccess,
			Unknown
		};

		class Hasher
		{
		public:

			// Возвращает истину если удалось посчитать хеш, в противном случае код ошибки.
			virtual HasherErrors calculateFileHash(std::wstring _filepath, std::string& _outHash);

			// Считает хеш бинарной последовательности, в случае успеха возвращает строку с вычисленным хешом.
			virtual std::string getHash(const char* _pData, const unsigned long  _sizeOfData) = 0;

			virtual ~Hasher(){}
		};
	}
}
