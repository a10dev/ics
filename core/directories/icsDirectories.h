

#pragma once

#include <map>
#include <boost/noncopyable.hpp>
#include "../../../../../thrift/cpp/ics/ics_types.h"
#include "directoryDescription.h"
#include "../../../helpers.h"
#include "../../../log.h"
#include "../icsSettings.h"
#include "../files/hasherCrc32.h"
#include "../files/hasherMd5.h"
#include "../files/hasherGost94.h"
#include "../files/hasherGost2012_256.h"
#include "../files/hasherGost2012_512.h"
#include "controlledDirsStorage.h"
#include "dirHasherImpl.h"
#include "../../../arm_events_client/event_emiter.h"

namespace ics
{
	namespace directories
	{
		typedef std::map<ics_service::HashAlg::type, files::Hasher*> HashersTable;
		typedef std::vector<ics::directories::DirectoryDescription> DirList;

		class DirControl : private boost::noncopyable
		{
		public:

			DirControl(logfile& _log);

			~DirControl();

			bool isCompromised() const;

			// Добавляет в область контроля целостности.
			bool add(std::wstring _dir, bool includeSubDirs, ics_service::HashAlg::type, ics_service::IcsStatusCode::type& result);

			// Удаляет из области контроля целостности.
			bool remove(std::wstring _dir);

			// Пересчитывает контрольную сумму для ранее добавленного объекта по-заданному алгоритму хеширования.
			bool changeHashAlgorithm(std::wstring _dir, ics_service::HashAlg::type _hashAlg, ics_service::IcsStatusCode::type& result);

			// Пересчитывает хеш для ранее добавленного директория, если новая сумма отличается от старой, об этом можно узнать в _isChanged.
			bool recalculate(std::wstring _dir, bool& _isChanged, ics_service::IcsStatusCode::type& result);

			// Пересчитывает контрольные суммы для всей области контроля целостности.
			bool recalculate();

			// Возвращает список контроллируемых директорий.
			void getList(DirsMap& _out) const;

			// Возвращает список каталогов, с нарушенной целостностью.
			void getListChanged(DirsMap& _out) const;

			// Возвращяет истину в случае если имеются объекты с нарушенной целостностью.
			bool hasChangedObjects() const;
			std::set<std::wstring> getMissed() const;
			bool isMissed(std::wstring _path) const;

			void flushData()
			{
				m_storage.flush();
			}

			// Возвращает список доступных алгоритмов хэширования
			std::vector<ics_service::HashAlg::type> getHashesList();

		private:
			logfile& m_log;

			files::HasherCrc32 m_hasherCrc32;
			files::HasherMd5 m_hasherMd5;
			files::HasherGost94 m_hasherGost94;
			files::HasherGost2012_256 m_hasherGost2012_256;
			files::HasherGost2012_512 m_hasherGost2012_512;
			HashersTable m_hashers;

			Storage m_storage;
		protected:
			bool calc(const std::wstring& _dir, bool _includeSubdirs, ics_service::HashAlg::type _algType, DirectoryDescription& _outInfo, ics_service::IcsStatusCode::type& result);

			bool hlpChangeHashAlgorithm(std::wstring _dir, ics_service::HashAlg::type _hashAlg, ics_service::IcsStatusCode::type& result);
			void markAsChanged(const DirectoryDescription& _info);
			void markAsMissed(std::wstring _dir);
			void markAsNormal(std::wstring _dir);

			std::shared_ptr<events_client::EventEmiter> getEventEmiter();
		};
	}
}
