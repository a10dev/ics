

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

			// ���������� ������ ���� ������� ��������� ���, � ��������� ������ ��� ������.
			virtual HasherErrors calculateFileHash(std::wstring _filepath, std::string& _outHash);

			// ������� ��� �������� ������������������, � ������ ������ ���������� ������ � ����������� �����.
			virtual std::string getHash(const char* _pData, const unsigned long  _sizeOfData) = 0;

			virtual ~Hasher(){}
		};
	}
}
