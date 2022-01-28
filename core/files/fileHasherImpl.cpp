

#pragma once

#include <stdint.h>
#define _WINSOCKAPI_
#include <windows.h>
#undef _WINSOCKAPI_
#include "../../../helpers.h"
#include "filehasherimpl.h"

namespace ics
{
	namespace files
	{
// 		std::string Hasher::calculateFileHash(std::wstring _filepath)
// 		{
// 			return "not implemented";
// 		}

		// Наследие Павла Саныча Обросова, херь та еще, избавимся в будущем.
		HasherErrors Hasher::calculateFileHash(std::wstring _filepath, std::string& _outHash)
		{
			std::string hash(""), hashesByFileParts("");

			// Offsets must be a multiple of the system's allocation granularity. We guarantee this by making our view size equal to the allocation granularity.
			SYSTEM_INFO sysInfo = { 0 }; // system information; used to get granularity
			GetSystemInfo(&sysInfo);
			DWORD dwSysGran = sysInfo.dwAllocationGranularity;

			HANDLE hFile = CreateFileW(_filepath.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				if (!windir::isFilePresent(_filepath))
				{
					return HasherErrors::Missed;
				}
				return HasherErrors::LockedAccess;
			}

			LARGE_INTEGER liFileSize = { 0 }; // temporary storage for file sizes for big files
			if (!GetFileSizeEx(hFile, &liFileSize))
			{
				return HasherErrors::Unknown;
			}

			const uint64_t cbFile = static_cast<uint64_t>(liFileSize.QuadPart);
			if (cbFile == 0) // Пустой файл!
			{
				CloseHandle(hFile);
				const char buf[10] = { 0 };
				_outHash = getHash(buf, 0);
				return NoError;
			}

			HANDLE hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
			CloseHandle(hFile);

			if (hMap == NULL)
			{
				return HasherErrors::Unknown;
			}

			for (uint64_t offset = 0, i = 0; offset < cbFile; offset += dwSysGran, i++)
			{
				DWORD high = static_cast<DWORD>((offset >> 32) & 0xFFFFFFFFul);
				DWORD low = static_cast<DWORD>(offset & 0xFFFFFFFFul);

				// The last view segment may be shorter.
				if (offset + dwSysGran > cbFile)
				{
					dwSysGran = static_cast<int>(cbFile - offset);
				}

				PBYTE pView = static_cast<PBYTE>(MapViewOfFile(hMap, FILE_MAP_READ, high, low, dwSysGran));
				if (pView)
				{
					hash = this->getHash((const char*)pView, dwSysGran);
					hashesByFileParts.append(hash);
					UnmapViewOfFile(pView);
				}
				else
				{
					UnmapViewOfFile(pView);
					CloseHandle(hMap);
					return HasherErrors::Unknown;
				}
			}

			CloseHandle(hMap);
			_outHash = getHash(hashesByFileParts.data(), hashesByFileParts.size());
			return HasherErrors::NoError;
		}
	}
}
