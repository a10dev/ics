

#pragma once

#include "hasher.h"
#include "../../../../CryptoCommon/CryptoLib/src/cryptoLib.h"

namespace ics
{
	namespace registry
	{
		class HasherMd5 : public Hasher
		{
		public:

			virtual std::string getHash(const char* _pData, const unsigned long  _sizeOfData) override
			{
				std::string inData(_pData, _sizeOfData);
				return crypto::GetMD5(inData);
			}
		};
	}
}