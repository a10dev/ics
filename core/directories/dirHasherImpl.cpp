

#pragma once

#include <stdint.h>
#define _WINSOCKAPI_
#include <windows.h>
#undef _WINSOCKAPI_
#include "../../../helpers.h"
#include "dirhasherimpl.h"

namespace ics
{
	namespace directories
	{
		typedef std::set<std::wstring> FsObjects;

		// Исправленный код Паши, в последствеии от него нужно избавиться.
		// Написать новый метод сбора информации о папках и файлах.
		bool Hasher::calculateDirHash(std::string& _calculatedHash, std::wstring _dirpath, bool _includeSubdirs, UINT64& totalSize)
		{
			FsObjects fsObjectsList;

			bool isFileExist = windir::isFilePresent(_dirpath);
			if (isFileExist)
			{
				auto result = windir::getNamesOfObjInDir(fsObjectsList, _dirpath, L"", true, _includeSubdirs, m_log, windir::SEARCH_FILES_AND_DIRS);
				if (result != 0)
				{
					m_log.print(std::string(__FUNCTION__) + " error - couldn't build list of files in folder " + strings::ws_s(_dirpath));
					return false;
				}
			}
			else
			{
				m_log.print(std::string(__FUNCTION__) + " error - folder is not present " + strings::ws_s(_dirpath));
				return false;
			}

			std::wstring bufferNames;
			std::string hashesByNames, hashesByValues, bufferValues;
			const std::size_t bufferLimit = 1024 * 1024 * 1; // 1 Mb.
			bufferNames.reserve(bufferLimit);
			bufferValues.reserve(bufferLimit);
			hashesByNames.reserve(1024 * 1024); // 1 Mb.
			hashesByValues.reserve(1024 * 1024);

			for (auto fsObject : fsObjectsList)
			{
				// Накопить во временном буфере данные, затем посчичать их хэш, делать порционно.
				bufferNames.append(fsObject);

				// Вычисляем хэш от содержимого каждый раз и добавляем в переменную
				if (fsObject[0] != L'*') // если в начале имени стоит символ "*", значит это папка
				{
					std::string fileBodyHash;
					if (m_fileHasher.calculateFileHash(fsObject, fileBodyHash) == files::HasherErrors::NoError)
					{
						bufferValues.append(fileBodyHash);
					}

					size_t fileSize = 0;
					if (windir::getFileByteSize(fsObject, fileSize))
					{
						totalSize += fileSize;
					}
				}

				// В буфере имеется достаточный объём данных, чтобы посчитать хеш.
				if (bufferNames.size() >= bufferLimit * 0.9)
				{
					hashesByNames.append(m_fileHasher.getHash((const char*)bufferNames.data(), bufferNames.size() * sizeof(std::wstring::value_type)));
					bufferNames.clear();
				}

				if (bufferValues.size() >= bufferLimit * 0.9)
				{
					hashesByValues.append(m_fileHasher.getHash(bufferValues.data(), bufferValues.size()));
					bufferValues.clear();
				}
			}

			// Есть данные, хеш которых посчитан не был.
			if (!bufferNames.empty())
			{
				hashesByNames.append(m_fileHasher.getHash((const char*)bufferNames.data(), bufferNames.size() * sizeof(std::wstring::value_type)));
			}

			if (!bufferValues.empty())
			{
				hashesByValues.append(m_fileHasher.getHash(bufferValues.data(), bufferValues.size()));
			}

			// Перевести все хеши в верхний регистр.
			auto tmpHash = m_fileHasher.getHash(hashesByNames.data(), hashesByNames.size()) + m_fileHasher.getHash(hashesByValues.data(), hashesByValues.size());
			_calculatedHash = m_fileHasher.getHash(tmpHash.data(), tmpHash.size());
			strings::toUpper(_calculatedHash);
			return true;
		}
	}
}
