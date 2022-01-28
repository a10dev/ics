
#pragma once

#include <atomic>
#include <thread>

#include "registry/icsRegistry.h"
#include "files/icsFiles.h"
#include "directories/icsDirectories.h"
#include "boot/icsBoot.h"
#include <boost/noncopyable.hpp>
#include "../../../../../thrift/cpp/ics/ics_types.h"
#include "../../management/serviceInternals.h"

namespace ics
{
	class Core : private boost::noncopyable
	{
	public:
		Core(std::string _configFile = SRV_STATE_A("ics.settings"), std::string logfilepath = SRV_LOG_A("szi_ics.log"));
		~Core();

		registry::RegControl& regControl();
		files::FilesControl& filesControl();
		directories::DirControl& dirControl();
		boot::BootControl& bootControl();

		Settings& getConfig();
		SYSTEMTIME getLastFullVerificationTime() const;
		
		// Сейчас просто копирует все настройки.
		// Должен обновлять конфигурацию работающего приложения, вносить изменения в планировщик.
		bool applySettings(const ics_service::IcsSettings& _settings);
		bool applyBootSettings(const ics_service::AutoVerification& _settings);

		static void recalculateRegFileDir(Core& _ics);
		static void recalculateBoot(Core& _ics);
		
		void updateLastFullVerificationTime();

		ics_service::IcsStatusCode::type getAllowedHashAlgs(std::vector<ics_service::HashAlg::type>& _list);

	private:
		// Выполняет поиск объектов с нарушеной целостностью.
		SYSTEMTIME m_lastFullIntegrityVerification; // Когда производилась полная проверка.
		std::mutex m_mtxLastVerTime;


		// Планировщик - осуществляет проверку целостности по времени.
		static void scheduler(DWORD _delayStartTimout, Core& _ics, DWORD _timeout /* in minutes.*/, const HANDLE& _eventObject, const std::function<void(Core&)>& _recalculateFunction);

		// Останавливает работу планировщика.
		void schedulerStop();
		void bootSchedulerStop();

		DWORD calculateTimeoutValue(unsigned long _hours, unsigned long _minutes);

		HANDLE m_hEventSettingsUpdated;
		HANDLE m_hEventBootSettingsUpdated;

		logfile m_log;
		Settings m_settings;

		registry::RegControl m_regControl;
		files::FilesControl m_filesControl;
		directories::DirControl m_dirControl;
		boot::BootControl m_bootControl;

		std::thread m_settingsThread;
		std::thread m_bootSettingsThread;
		std::thread m_recalculateRegFileDirThread;
		std::thread m_recalculateBootThread;
	};



	class IcsCoreBridge
	{
		// Valid while ics-server works.
		ics::Core* m_core;
		std::mutex m_lock;

	public:
		void lock(){
			m_lock.lock();
		}

		void unlock(){
			m_lock.unlock();
		}

		bool isValid() {
			return m_core != NULL;
		}

		ics::Core* get()
		{
			return m_core;
		}

		void setIcsCore(ics::Core* _icsCore)
		{
			m_core = _icsCore;
		}

		bool isCompromisedFileSystem()
		{
			if (m_core)
			{
				return (!m_core->filesControl().isCompromised()) && (!m_core->dirControl().isCompromised());
			}

			return false;
		}

		bool isCompromisedBoot()
		{
			if (m_core)
			{
				return (!m_core->bootControl().isCompromised());
			}

			return false;
		}

		bool isCompromisedRegistry()
		{
			if (m_core)
			{
				return (!m_core->regControl().isCompromised());
			}

			return false;
		}

		IcsCoreBridge( /*ics::Core* _core*/ ){  }
	};
}
