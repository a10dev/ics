#include "icsBoot.h"

namespace ics
{
	namespace boot
	{
	
		BootControl::BootControl(logfile& _log) : m_log(_log), m_storage(SRV_STATE_A("ics_boot.szi"))
		{
			m_hashers[ics_service::HashAlg::Crc_32] = &m_hasherCrc32;
			/*
			m_hashers[ics_service::HashAlg::Md_5] = &m_hasherMd5;
			m_hashers[ics_service::HashAlg::Gost_94] = &m_hasherGost94;
			m_hashers[ics_service::HashAlg::Gost_2012_256] = &m_hasherGost2012_256;
			m_hashers[ics_service::HashAlg::Gost_2012_512] = &m_hasherGost2012_512;
			*/

			if (!m_storage.find(wszDrive, m_bootDescriptor))
			{
				init(ics_service::HashAlg::Crc_32);
				m_storage.add(m_bootDescriptor);
			}						
		}

		BootControl::~BootControl()
		{
			flushData();
		}

		void BootControl::init(ics_service::HashAlg::type _hashAlg)
		{
			// при включении контроля считаем контрольную сумму сектора и запоминаем ее в m_bootDescriptor
			if (calc(static_cast<ics_service::HashAlg::type>(_hashAlg), m_bootDescriptor))
			{
				m_log.print(std::string(__FUNCTION__) + " Start Boot control.");
			}
			else
			{
				m_log.print(std::string(__FUNCTION__) + " Error while starting Boot control.");
			}
		}

		bool BootControl::isCompromised() const
		{
			return m_bootDescriptor.status == STATE_CHANGED;
		}

		/*
		bool BootControl::changeHashAlgorithm(ics_service::HashAlg::type _hashAlg)
		{
			if (calc(static_cast<ics_service::HashAlg::type>(_hashAlg), m_bootDescriptor))
			{
				m_log.print(std::string(__FUNCTION__) + " Boot control algorithm changed.");
				return true;
			}
			else
			{
				m_log.print(std::string(__FUNCTION__) + " Error while changing algorithm.");
				return false;
			}
		}
		*/

		bool BootControl::recalculate()
		{
			BootDescription info = {0};

			if (calc(static_cast<ics_service::HashAlg::type>(m_bootDescriptor.algorithm), info))
			{
				auto changed = memcmp(m_bootDescriptor.hash, info.hash, sizeof(m_bootDescriptor.hash)) != 0;
				if (changed)
				{
					m_bootDescriptor.status = STATE_CHANGED;
					
					///// Отправка события безопасности
					szi::OperationResult result;
					result.errorCode = szi::SziStatusCodes::NoError;
					result.status = "NoError";
					getEventEmiter()->IntCntrlBootBrk("system", result);
				}
				else
				{
					m_bootDescriptor.status = STATE_NORMAL;
				}

				m_bootDescriptor.time = info.time;
				m_storage.update(wszDrive, m_bootDescriptor);
				return true;
			}
			return false;
		}

		bool BootControl::calc(ics_service::HashAlg::type _algType, BootDescription& _outInfo)
		{
			char bootSectorBuffer[BOOT_SECTOR_SIZE];

			// Получаем массив байтов загрузочного сектора			
			if (!ReadVolumeBytes(bootSectorBuffer))
			{
				m_log.print(std::string(__FUNCTION__) + " error - couldn't read boot sector.");
				return false;
			}

			// Считаем контрольную сумму для конкретно выбранного алгоритма _algType
			auto hasher = m_hashers.find(_algType);
			if (hasher != m_hashers.end())
			{
				std::string hash = hasher->second->getHash(bootSectorBuffer, BOOT_SECTOR_SIZE);
				m_log.print(std::string(__FUNCTION__) + " hash: " + hash);

				// Заполняем описание загрузочного сектора
				BootDescription information = {0};
				information.status = STATE_NORMAL;
				information.version = 0;
				information.algorithm = _algType;
				GetLocalTime(&information.time);

				fill_chars(information.hash, hash.c_str());
				fill_wchars(information.szFilePath, wszDrive);

				_outInfo = information;
				return true;
			}
			else
			{
				m_log.print(std::string(__FUNCTION__) + " error - unknown hashing algorithm was passed.");
			}

			return false;
		}

		//
		// Чиатем BOOT_SECTOR_SIZE байтов устройства wszDrive
		//		
		bool BootControl::ReadVolumeBytes(char* buffer)
		{
			DWORD bytesRead  = 0;

			// инициализируем буффер 
			memset(buffer, 0, BOOT_SECTOR_SIZE);

			// открываем устройство на чтение
			HANDLE fileHandle = CreateFileW(wszDrive, 
											GENERIC_READ,  // открытие на чтение
											0,             // не расшаривать
											0,             // дефолтные аттрибуты безопасности
											OPEN_EXISTING, // открытие существующего ?
											0,             // дефолтные аттрибуты и флаги 
											0);            // без аттрибутов временного файла

			// проверяем что открыть получилось
			if (fileHandle == INVALID_HANDLE_VALUE)
			{
				auto lastError = GetLastError();
				m_log.print(std::string(__FUNCTION__) + " CreateFileW: last error: " + boost::lexical_cast<std::string>(lastError));
				return false;
			}

			// чиатем начало устройства, первые 512 байт, которые есть MBR
			const BOOL success = ReadFile(fileHandle, buffer, BOOT_SECTOR_SIZE, &bytesRead, NULL);
			if (!success)
			{
				auto lastError = GetLastError();
				m_log.print(std::string(__FUNCTION__) + "last error: " + boost::lexical_cast<std::string>(lastError));
			}

			// проверяем сигнатуру MBR, должен заканчиватся на 55h AAh
			if ((buffer[510] & 0xFF) == 0x55 && (buffer[511] & 0xFF) == 0xAA)
			{
				m_log.print(std::string(__FUNCTION__) + " MBR sector read succssfully." );
			}

			std::stringstream ss;
			ss << std::endl;
			for (auto i = 0; i < BOOT_SECTOR_SIZE; ++i)
			{	
				if (i > 0 && i % 32 == 0) ss << std::endl;
				ss << std::hex << std::setw(2) << std::setfill('0') << (buffer[i] & 0xff) << " ";				
			}				
			m_log.print(std::string(__FUNCTION__) + " boot dump: " + ss.str());


			// закрываем устройство
			CloseHandle(fileHandle);

			return (success && BOOT_SECTOR_SIZE == bytesRead);
		}

		BootDescription BootControl::getDescription() const
		{
			return m_bootDescriptor;
		}

		// Возвращает список доступных алгоритмов хэширования
		std::vector<ics_service::HashAlg::type> BootControl::getHashesList()
		{
			std::vector<ics_service::HashAlg::type> _list;
			std::string testString = "Lorem ipsum dolor sit amet";

			for (std::pair<ics_service::HashAlg::type, files::Hasher*> hashPair : m_hashers)
			{
				std::string hash = hashPair.second->getHash(testString.c_str(), testString.size());
				if (hash.size() > 0)
				{
					_list.push_back(hashPair.first);
				}
			}

			return _list;
		}

		std::shared_ptr<events_client::EventEmiter> BootControl::getEventEmiter()
		{
			return events_client::EventEmiterHolder::get("127.0.0.1",
			                                             szi::Settings::get().getSziEventsLocalPort(m_log.getLogFilePath()),
			                                             m_log.getLogFilePath());
		}
	}
}
