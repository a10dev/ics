

#pragma once

#include "../icsInternalTypes.h"
#define _WINSOCKAPI_
#include <windows.h>
#undef _WINSOCKAPI_

// Limit for path size of registry key is 300 symbols. 
#define MAX_KEY_PATH			300

namespace ics
{
	namespace registry
	{


#pragma pack(push, 1)

		struct RegistryKeyDescription
		{
			DWORD version;
			//ics_service::HashAlg::type algorithm;
			DWORD algorithm;
			bool subKeysIncluded;
			DWORD countObjects;
			SYSTEMTIME time;
			wchar_t szKeyPath[MAX_KEY_PATH]; // String in UPPER case with last '\0' null symbol.
			char hash[512]; // Ansi string in UPPER case without last '\0' symbol.
			ics::IntegrityState status; // Новое поле для Димы.

			// Сравнивает структуры побитово, игнорируя временные метки.
			bool operator==(const RegistryKeyDescription& _r) const
			{
				RegistryKeyDescription me = *this, obj = _r;
				ZeroMemory(&me.time, sizeof me.time);
				ZeroMemory(&obj.time, sizeof obj.time);
				ZeroMemory(&obj.status, sizeof obj.status);

				return memcmp(&me, &obj, sizeof(RegistryKeyDescription)) == 0;
			}

			//
			// Returns following values:
			// error: -1 if hash algorithms are different.
			// 0 when hashes are aqual;
			// 1 if hashes are not equal; 
			//
			int compareHashes(const RegistryKeyDescription& _r) const
			{
				if (this->algorithm != _r.algorithm)
				{
					return -1;
				}

				RegistryKeyDescription me = *this, obj = _r;
				ZeroMemory(&me.time, sizeof me.time);
				ZeroMemory(&obj.time, sizeof obj.time);
				ZeroMemory(&obj.status, sizeof obj.status);

				bool equal = memcmp(&me, &obj, sizeof(RegistryKeyDescription)) == 0;
				return equal ? 0 : 1;
			}


		};
	}

#pragma pack(pop)

}
