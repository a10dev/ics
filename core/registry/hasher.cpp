

#pragma once

//#include "../../../../../thrift/cpp/ics/ics_types.h"
//#include "parser.h"
#include "../../../helpers.h"
#include "hasher.h"

namespace ics
{
	namespace registry
	{
		std::string Hasher::calculateKey(const KeyValues& _keyValuesList)
		{
			std::string hashesByNames, hashesByValues, hash, resCommonHash;
			std::size_t bufferLimit = 1024 * 1024 * 1; // 1 Mb.

			std::string bufferNames, bufferValues;
			bufferNames.reserve(bufferLimit);
			bufferValues.reserve(bufferLimit);

			hashesByNames.reserve(1024 * 1024); // 1 Mb.
			hashesByValues.reserve(1024 * 1024);

			for (auto i : _keyValuesList)
			{
				// Накопить во временном буфере данные, затем посчичать их хэш, делать порционно.
				bufferNames.append(i.second.name);
				bufferValues.append(i.second.data);

				// В буфере имеется достаточный объём данных, чтобы посчитать хеш.
				if (bufferNames.size() >= bufferLimit * 0.9)
				{
					hash = getHash(reinterpret_cast<const char*>(bufferNames.data()), bufferNames.size() * sizeof(std::string::value_type));

					hashesByNames.append(hash);
					bufferNames.clear();
				}

				if (bufferValues.size() >= bufferLimit * 0.9)
				{
					hash = getHash(reinterpret_cast<const char*>(bufferValues.data()), bufferValues.size() * sizeof(std::string::value_type));

					hashesByValues.append(hash);
					bufferValues.clear();
				}
			}

			// Есть данные, хеш которых посчитан не был.
			if (!bufferNames.empty())
			{
				hash = getHash(reinterpret_cast<const char*>(bufferNames.data()), bufferNames.size() * sizeof(std::string::value_type));
				hashesByNames.append(hash);
			}
			if (!bufferValues.empty())
			{
				hash = getHash(reinterpret_cast<const char*>(bufferValues.data()), bufferValues.size() * sizeof(std::string::value_type));
				hashesByValues.append(hash);
			}

			// Посчитать хеш хешей.
			hash = getHash(hashesByNames.data(), hashesByNames.size()) + getHash(hashesByValues.data(), hashesByValues.size());
			resCommonHash = getHash(hash.data(), hash.size());

			// Перевести все хеши в верхний регистр.
			strings::toUpper(resCommonHash);
			return resCommonHash;
		}
	}
}
