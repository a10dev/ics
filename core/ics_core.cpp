

#include "ics_core.h"
#include <thread>
#define _WINSOCKAPI_
#include <windows.h>
#undef _WINSOCKAPI_
#include "../../management/serviceInternals.h"

namespace ics
{
	Core::Core(std::string _configFile,
		std::string logfilepath)
		:
		m_log(logfilepath),
		m_regControl(m_log),
		m_filesControl(m_log),
		m_dirControl(m_log),
		m_bootControl(m_log),
		m_settings(_configFile),
		m_lastFullIntegrityVerification({ 0 }),
		m_hEventSettingsUpdated(NULL),
		m_hEventBootSettingsUpdated(NULL)
	{
		m_log.print(std::string(__FUNCTION__) + ": ICS engine is constructing");

		// Объект создается in non-signal state с автосбросом.
		m_hEventSettingsUpdated = CreateEventW(NULL, FALSE, FALSE, NULL);
		m_hEventBootSettingsUpdated = CreateEventW(NULL, FALSE, FALSE, NULL);

		const auto settings = m_settings.get();
		//
		// Запустить проверку целостности контролируемых объектов, если того требует конфигурация.
		//
		if (settings.enablePeriodScan)
		{			
			DWORD delay = calculateTimeoutValue(settings.startTimeHH, settings.startTimeMM);

			if (settings.autostart)
			{
				delay = 0; //запуск при старте ОС, не ждем, запускаем сразу
			}

			// Перед тем, как запустить периодическую проверку, требуется расчитать время 
			// для начала работы планировщика.
			m_log.print(std::string(__FUNCTION__) + ": start sheduling for Reg File Dir with delay = " + std::to_string(delay) + " and timeout = " + std::to_string(settings.period_scan));
			// Запустить в отдельном потоке периодическое сканирование.
			std::thread(Core::scheduler, delay, std::ref(*this), settings.period_scan, m_hEventSettingsUpdated, Core::recalculateRegFileDir).detach();
		} 
		else if (settings.autostart)
		{
			std::thread(Core::recalculateRegFileDir, std::ref(*this)).detach();
		}

		//
		// Запустить проверку целостности загрузочного сектора, если того требует конфигурация.
		//
		auto verificationTimeout = szi::Settings::get().icsBootGetVerificationTimeout();
		auto delay = verificationTimeout;

		if (szi::Settings::get().icsBootIsEnabledAutoScan())
		{
			delay = 0; //запуск при старте ОС, не ждем, запускаем сразу
		}
		
		std::thread(Core::scheduler, delay, std::ref(*this), verificationTimeout, m_hEventBootSettingsUpdated, Core::recalculateBoot).detach();		
	}

	Core::~Core()
	{
		m_log.print(std::string(__FUNCTION__) + ":  _ITD_ ICS engine is destroying");

		if (m_hEventSettingsUpdated != NULL)
		{
			SetEvent(m_hEventSettingsUpdated);
			CloseHandle(m_hEventSettingsUpdated);
		}

		if (m_hEventBootSettingsUpdated != NULL)
		{
			SetEvent(m_hEventBootSettingsUpdated);
			CloseHandle(m_hEventBootSettingsUpdated);			
		}

		regControl().flushData();
		filesControl().flushData();
		dirControl().flushData();
		bootControl().flushData();

		m_log.print(std::string(__FUNCTION__) + ": _ITD_ internal information has flushed.");
	}

	registry::RegControl& Core::regControl()
	{
		return m_regControl;
	}

	files::FilesControl& Core::filesControl()
	{
		return m_filesControl;
	}

	directories::DirControl& Core::dirControl()
	{
		return m_dirControl;
	}

	boot::BootControl& Core::bootControl()
	{
		return m_bootControl;
	}
	
	Settings& Core::getConfig()
	{
		return m_settings;
	}

	SYSTEMTIME Core::getLastFullVerificationTime() const
	{
		return m_lastFullIntegrityVerification;
	}

	void Core::updateLastFullVerificationTime()
	{
		m_log.print(std::string(__FUNCTION__));
		std::unique_lock<std::mutex> mlock(m_mtxLastVerTime);

		SYSTEMTIME time;
		GetLocalTime(&time);
		m_lastFullIntegrityVerification = time;
	}

	ics_service::IcsStatusCode::type Core::getAllowedHashAlgs(std::vector<ics_service::HashAlg::type>& _list)
	{
		std::vector<ics_service::HashAlg::type> regHashesList = m_regControl.getHashesList();
		std::vector<ics_service::HashAlg::type> filesHashesList = m_filesControl.getHashesList();
		std::vector<ics_service::HashAlg::type> dirHashesList = m_dirControl.getHashesList();
		std::vector<ics_service::HashAlg::type> bootHashesList = m_bootControl.getHashesList();

		std::vector<ics_service::HashAlg::type> tmp_list1, tmp_list2;
		std::set_union(regHashesList.begin(), regHashesList.end(), filesHashesList.begin(), filesHashesList.end(), std::back_inserter(tmp_list1));
		std::set_union(dirHashesList.begin(), dirHashesList.end(), bootHashesList.begin(), bootHashesList.end(), std::back_inserter(tmp_list2));
		std::set_union(tmp_list1.begin(), tmp_list1.end(), tmp_list2.begin(), tmp_list2.end(), std::back_inserter(_list));

		return ics_service::IcsStatusCode::Ics_Success;
	}

	bool Core::applySettings(const ics_service::IcsSettings& _settings)
	{
		if (_settings.enablePeriodScan)
		{
			//
			// Если произошли изменения по времени проверки целостности, сформировать новый 
			// планировщик по осуществлению периодического сканирования.
			//
			if (false ==
				(_settings.startTimeHH == m_settings.get().startTimeHH &&
				_settings.startTimeMM == m_settings.get().startTimeMM &&
				_settings.period_scan == m_settings.get().period_scan))
			{

				if (m_settings.get().enablePeriodScan)
				{
					// посылаем событие останова через объект m_hEventSettingsUpdated, так как существует уже запущеный шедулинг
					schedulerStop();
				}
				
				DWORD delay = calculateTimeoutValue(_settings.startTimeHH, _settings.startTimeMM);

				// Перед тем, как запустить периодическую проверку, требуется расчитать время 
				// для начала работы планировщика.
				m_log.print(std::string(__FUNCTION__) + ": start sheduling for Reg File Dir with delay = " + std::to_string(delay) + " and timeout = " + std::to_string(_settings.period_scan));
				// Запустить в отдельном потоке периодическое сканирование.
				std::thread(Core::scheduler, delay, std::ref(*this), _settings.period_scan, m_hEventSettingsUpdated, Core::recalculateRegFileDir).detach();
			}
		}
		else
		{
			// Отменить запланированую проверку целостности.
			schedulerStop();
		}
		
		m_settings.update(_settings);
		return true;
	}

	bool Core::applyBootSettings(const ics_service::AutoVerification& _settings)
	{
		// если установлена запланировання проверка целостности загрузочного сектора
		if (_settings.repeatTimeout)
		{
			//
			// Если время поменялось то перезпускаем планировщик
			//
			if (_settings.repeatTimeout != szi::Settings::get().icsBootGetVerificationTimeout())
			{
				// посылаем событие останова через объект m_hEventBootSettingsUpdated
				bootSchedulerStop();

				DWORD timeout = 0; 
				
				// Запустить в отдельном потоке периодическое сканирование загрузочного сектора.
				std::thread(Core::scheduler, timeout, std::ref(*this), _settings.repeatTimeout, m_hEventBootSettingsUpdated, Core::recalculateBoot).detach();
			}
		}
		else
		{
			// Отменить запланированую проверку целостности.
			bootSchedulerStop();
		}
		
		// сохраняем новые настройки в szi.conf
		szi::Settings::get().icsBootSetVerificationTimeout(_settings.repeatTimeout);
		szi::Settings::get().icsBootEnableAutoScan(_settings.autostart);
		szi::Settings::get().icsBootSetCompromisedValue((_settings.action == ics_service::AutoAction::LockSystem) ? "lock" : "ignore");
		
		return true;
	}

	DWORD Core::calculateTimeoutValue(unsigned long _hours, unsigned long _minutes)
	{
		DWORD waitMinutes{};
		SYSTEMTIME currentTime{};
		GetLocalTime(&currentTime);

		const unsigned long currentMinutes = currentTime.wHour * 60 + currentTime.wMinute;
		const unsigned long settingsMinutes = _hours * 60 + _minutes;

		if (currentMinutes <= settingsMinutes)
		{
			return (settingsMinutes - currentMinutes);
		}
		
		return 0;
	}

	void Core::schedulerStop()
	{
		if (m_hEventSettingsUpdated != NULL)
		{
			SetEvent(m_hEventSettingsUpdated);
		}
	}

	void Core::bootSchedulerStop()
	{
		if (m_hEventBootSettingsUpdated != NULL)
		{
			SetEvent(m_hEventBootSettingsUpdated);
		}
	}

	void Core::scheduler(DWORD _delayStartTimeout, Core& _ics, DWORD _timeout, const HANDLE& _eventObject, const std::function<void(Core&)>& _recalculateFunction)
	{
		// Небезопасное использование объекта Core, требуется добавить проверку на существование.
		// ...

		// 1. Период ожидания перед тем как приступить к поиску изменений.
		// Нужно просто прождать какое-то время перед тем как приступить к периодическим проверкам.
		DWORD wakeupReason = WaitForSingleObject(_eventObject, _delayStartTimeout * 1000 * 60);

		if (wakeupReason == WAIT_OBJECT_0) // Прекратить работу, в случае если конфигурация обновилась.
		{
			_ics.m_log.print(std::string(__FUNCTION__) + ": sheduling thread " + std::to_string(GetCurrentThreadId()) +  "stopped after delay.");
			return;
		}

		// Так как событие с автосбросом нужно его снова явно перевести в сигнальное состояние,
		// чтобы приступить к первому этапу сканироввания.
		// Второй вариант - запустить самостоятельно поиск изменений без модификаии события.
		
		// /* 1st */ SetEvent(_ics.m_hEventSettingsUpdated);
		/* 2nd way. */ 	
		std::thread(_recalculateFunction, std::ref(_ics)).detach();

		while (true)
		{
			wakeupReason = WaitForSingleObject(_eventObject, _timeout * 1000 * 60);

			switch (wakeupReason)
			{
			case WAIT_OBJECT_0:
				// Настройки изменились, требуется завершиться.
				_ics.m_log.print(std::string(__FUNCTION__) + ": sheduling thread "  + std::to_string(GetCurrentThreadId()) + "  stopped after wakeup.");				
				return;

			case WAIT_TIMEOUT:
				// Пришло время для проверки целостности.
				std::thread(_recalculateFunction, std::ref(_ics)).detach();
				break;

			default:
				//return;
				break;
			}
		}		
	}

	void Core::recalculateRegFileDir(Core& _ics)
	{
		//
		// Небезопасное использование объекта, требуется добавить проверку на существование.
		// ...		

		_ics.m_log.print(std::string(__FUNCTION__) + ": executing recalculate Reg File Dir...");
		_ics.regControl().recalculate();
		_ics.filesControl().recalculate();
		_ics.dirControl().recalculate();

		_ics.updateLastFullVerificationTime();
	}

	void Core::recalculateBoot(Core& _ics)
	{
		_ics.m_log.print(std::string(__FUNCTION__) + ": executing recalculate Boot sector...");
		_ics.bootControl().recalculate();
	}

}
