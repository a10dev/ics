

#pragma once

#include <map>
#include <boost/noncopyable.hpp>
#include "../../../../../thrift/cpp/ics/ics_types.h"
#include "fileDescription.h"
#include "../../../helpers.h"
#include "../../../log.h"
#include "../icsSettings.h"
#include "hasherCrc32.h"
#include "hasherMd5.h"
#include "hasherGost94.h"
#include "hasherGost2012_256.h"
#include "hasherGost2012_512.h"
#include "controlledFilesStorage.h"
#include "../../../arm_events_client/event_emiter.h"

namespace ics
{
	namespace files
	{
		typedef std::map<ics_service::HashAlg::type, Hasher*> HashersTable;
		typedef std::vector<ics::files::FileDescription> FilesList;
		typedef std::set<std::wstring> StrSet;

		class FilesControl : private boost::noncopyable
		{
		public:

			FilesControl(logfile& _log);

			~FilesControl();

			// Говорит о целостности контролируемых объектов.
			bool isCompromised() const;

			// Добавляет в область контроля целостности файл.
			bool add(std::wstring _file, ics_service::HashAlg::type _algorithm, ics_service::IcsStatusCode::type& result);

			// Удаляет из области контроля целостности.
			bool remove(std::wstring _file);

			// Пересчитывает контрольную сумму для ранее добавленного файла по-заданному алгоритму хеширования.
			bool changeHashAlgorithm(std::wstring _file, ics_service::HashAlg::type _hashAlg, ics_service::IcsStatusCode::type& result);

			// Пересчитывает хеш для ранее добавленного файла, если новая сумма отличается от старой
			// то об этом можно узнать в _isChanged.
			bool recalculate(std::wstring _file, bool& _isChanged, ics_service::IcsStatusCode::type& result);

			// Пересчитывает контрольные суммы для всей области контроля целостности.
			bool recalculate();

			// Возвращает список контроллируемых файлов.
			void getList(FilesMap& _outFiles) const;

			// Возвращает список файлов, с нарушенной целостностью.
			void getListChanged(FilesMap& _outFiles) const;
			bool hasChangedObjects() const;
			void getMissed(std::set<std::wstring>& _outMissedFiles) const;
			bool hasMissedFiles() const;
			bool isMissed(std::wstring _file);
			void getLocked(std::set<std::wstring>& _outFiles) const;
			bool isLocked(std::wstring _file);

			void flushData()
			{
				m_storage.flush();
			}

			// Возвращает список доступных алгоритмов хэширования
			std::vector<ics_service::HashAlg::type> getHashesList();

		private:
			logfile& m_log;

			HasherCrc32 m_hasherCrc32;
			HasherMd5 m_hasherMd5;
			HasherGost94 m_hasherGost94;
			HasherGost2012_256 m_hasherGost2012_256;
			HasherGost2012_512 m_hasherGost2012_512;
			HashersTable m_hashers;

			Storage m_storage;

		protected:
			bool calc(const std::wstring& _file, ics_service::HashAlg::type _algType, FileDescription& _outInfo, ics_service::IcsStatusCode::type& result);

			bool hlpChangeHashAlgorithm(const std::wstring& _file, ics_service::HashAlg::type _hashAlg, ics_service::IcsStatusCode::type& result);
			void markAsChanged(const FileDescription& _fileInfo);
			void markAsLocked(const std::wstring& _file);
			void markAsMissed(const std::wstring& _file);
			void markAsNormal(const std::wstring& _file);

			std::shared_ptr<events_client::EventEmiter> getEventEmiter();
		};
	}
}
