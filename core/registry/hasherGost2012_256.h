

#pragma once

#include "hasher.h"
#include "../../../../CryptoCommon/CryptoLib/src/cryptoLib.h"

namespace ics
{
	namespace registry
	{
		class HasherGost2012_256 : public Hasher
		{
		public:

			virtual std::string getHash(const char* _pData, const unsigned long  _sizeOfData) override
			{
				std::string inData(_pData, _sizeOfData);
				std::string outHash;
				if (crypto::cpGetGost2012_256(inData, outHash))
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