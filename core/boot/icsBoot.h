

#pragma once

#include <map>
#include <boost/noncopyable.hpp>
#include "../../../../../thrift/cpp/ics/ics_types.h"
#include "../../../log.h"
#include "../files/fileDescription.h"
#include "../files/hasherCrc32.h"
#include "../files/hasherMd5.h"
#include "../files/hasherGost94.h"
#include "../files/hasherGost2012_256.h"
#include "../files/hasherGost2012_512.h"
#include "../files/controlledFilesStorage.h"
#include "../../../arm_events_client/event_emiter.h"
#include "../../../helpers.h"
#include "../../../management/settings/sziLocalSettings.h"

#include <windows.h>

#define BOOT_SECTOR_SIZE 512
#define VOLUME   L"\\\\.\\C:"
#define wszDrive L"\\\\.\\PhysicalDrive0"

namespace ics
{
	namespace boot
	{
		typedef std::map<ics_service::HashAlg::type, files::Hasher*> HashersTable;
		typedef ics::files::FileDescription BootDescription;

		class BootControl : private boost::noncopyable
		{
		public:
			BootControl(logfile& _log);

			~BootControl();

			// Включаем контроль за загрузочным сектором
			void init(ics_service::HashAlg::type _hashAlg);

			// Говорит о целостности / испорченности загрузочного сектора.
			bool isCompromised() const;

			// Пересчитывает контрольную сумму для ранее добавленного файла по-заданному алгоритму хеширования.
			//bool changeHashAlgorithm(ics_service::HashAlg::type _hashAlg);

			// Пересчитывает контрольные суммы для всей области контроля целостности.
			bool recalculate();

			// информация по бут сектору
			BootDescription getDescription() const;

			void flushData()
			{
				m_storage.flush();
			}

			// Возвращает список доступных алгоритмов хэширования
			std::vector<ics_service::HashAlg::type> getHashesList();

		private:
			logfile& m_log;

			files::HasherCrc32 m_hasherCrc32;
			/*
			files::HasherMd5 m_hasherMd5;
			files::HasherGost94 m_hasherGost94;
			files::HasherGost2012_256 m_hasherGost2012_256;
			files::HasherGost2012_512 m_hasherGost2012_512;
			*/
			HashersTable m_hashers;

			BootDescription m_bootDescriptor;

			ics::files::Storage m_storage;
			
		protected:
			bool ReadVolumeBytes(char* buffer);
			bool calc(ics_service::HashAlg::type _algType, BootDescription& _outInfo);
			std::shared_ptr<events_client::EventEmiter> getEventEmiter();
		};
	}
}
