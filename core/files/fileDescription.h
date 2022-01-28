

#pragma once

#define _WINSOCKAPI_
#include <windows.h>
#undef _WINSOCKAPI_
#include "../icsInternalTypes.h"

namespace ics
{
	namespace files
	{

#pragma pack(push, 1)

		struct FileDescription
		{
			DWORD version;
			DWORD algorithm; // ics_service::HashAlg::type algorithm;
			SYSTEMTIME time;
			int64_t fileSize;
			wchar_t szFilePath[1024]; // String in UPPER case with last '\0' null symbol.
			char hash[512]; // Ansi string in UPPER case without last '\0' symbol.
			ics::IntegrityState status; // Новое поле для Димы.


			// Сравнивает структуры побитово, игнорируя некоторые метки.
			bool operator==(const FileDescription& _r) const
			{
				FileDescription me = *this, obj = _r;
				ZeroMemory(&me.time, sizeof me.time);
				ZeroMemory(&obj.time, sizeof obj.time);
				ZeroMemory(&obj.status, sizeof obj.status);

				return memcmp(&me, &obj, sizeof(FileDescription)) == 0;
			}

			//
			// Returns following values:
			// error: -1 if hash algorithms are different.
			// 0 when hashes are aqual;
			// 1 if hashes are not equal; 
			//
			int compareHashes(const FileDescription& _r) const
			{
				if (this->algorithm != _r.algorithm)
				{
					return -1;
				}

				FileDescription me = *this, obj = _r;
				ZeroMemory(&me.time, sizeof me.time);
				ZeroMemory(&obj.time, sizeof obj.time);
				ZeroMemory(&obj.status, sizeof obj.status);

				bool equal = memcmp(&me, &obj, sizeof(FileDescription)) == 0;
				return equal ? 0 : 1;
			}

		private:
			bool hlpCompare(FileDescription& _r)
			{
				FileDescription me = *this, obj = _r;
				ZeroMemory(&me.time, sizeof me.time);
				ZeroMemory(&obj.time, sizeof obj.time);
				ZeroMemory(&obj.status, sizeof obj.status);

				return memcmp(&me, &obj, sizeof(FileDescription)) == 0;
			}
		};
	}

#pragma pack(pop)

}
