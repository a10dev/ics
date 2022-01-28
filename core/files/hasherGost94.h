

#pragma once

#include "filehasherimpl.h"
#include "../../../../CryptoCommon/CryptoLib/src/cryptoLib.h"

namespace ics
{
	namespace files
	{
		class HasherGost94 : public Hasher
		{
		public:

			virtual std::string getHash(const char* _pData, const unsigned long  _sizeOfData) override
			{
				// CryptoPro gost 94-year.
				std::string inData(_pData, _sizeOfData);
				std::string outHash;
				if (crypto::cpGetGost9411(inData, outHash))
				{
					return outHash;
				}
				else
				{
					return std::string();
				}
			}
		};
	}
}
