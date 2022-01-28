

#pragma once

#include <string>
#include "../files/fileHasherImpl.h"
#include "../../../log.h"

namespace ics
{
	namespace directories
	{
		class Hasher
		{
		public:

			Hasher(files::Hasher& _fileHasher, logfile& _log) :m_fileHasher(_fileHasher), m_log(_log){}

			// Строит иерархию всех объектов каталога и вычисляет итоговый хеш на основе структуры каталога
			// и содержимого всех файлов.
			virtual bool calculateDirHash(std::string& _calculatedHash, std::wstring _dirpath, bool _includeSubdirs, UINT64 &totalSize );

			virtual ~Hasher(){}

		private:
			// Расчётом хеша конечного файла занимается специальный объект.
			files::Hasher& m_fileHasher;
			logfile& m_log;
		};
	}
}
