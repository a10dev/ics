

#pragma once
#include "parser.h"

namespace ics
{
	namespace registry
	{
		class Hasher
		{
		public:

			// ������ ����� ������, ��������� ��� �� ��� ������������ ������ ������ (���������),
			// ���������� ����������� ��������� �������.
			// � ������ ������, ��������� ������������ � _outHash ���������.
			virtual std::string calculateKey(const KeyValues& keyValuesList);

			// ������� ��� �������� ������������������, � ������ ������ ���������� ������ � ����������� �����.
			virtual std::string getHash(const char* _pData, const unsigned long  _sizeOfData) = 0;

			virtual ~Hasher(){}
		};
	}
}
