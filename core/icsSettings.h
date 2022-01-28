

#pragma once

#include "../../../../../thrift/cpp/ics/ics_types.h"
#include "../../tstorage.h"

namespace ics
{
	typedef TStorage<ics_service::IcsSettings, NoSelector<ics_service::IcsSettings> > SettingsStorage;

	struct Settings
	{
	public:

		//
		// ��������� ���� � ����� �� ����� � ����������� � ������� �������� �����������. 
		//
		Settings(std::string _settingsFilePath = "ics.settings") :m_config(_settingsFilePath) {}

		//
		// ��������� ��������� � ����������� � ��������� � ����� �� �����.
		//
		Settings(const ics_service::IcsSettings& _applyNewSettings, std::string _settingsFilePath = "ics.settings") :m_config(_settingsFilePath)
		{
			update(_applyNewSettings);
		}
		
		void update(const ics_service::IcsSettings& _settings)
		{
			SettingsStorage storage(m_config);
			storage.clear();
			storage.push_back(_settings);
		}

		ics_service::IcsSettings get() const
		{
			SettingsStorage storage(m_config);

			ics_service::IcsSettings config;
			if (!storage.empty())
			{
				config = storage.at(0);
			}
			else
			{
				config.enablePeriodScan = false;
				config.autostart = false;
				config.period_scan = 100; // �������� �� ��������� ��� �� ������ � �������
				config.startTimeHH = 12;
				config.startTimeMM = 30;
				config.auto_action = ics_service::AutoAction::Ignore;
				config.bootAutoAction = ics_service::AutoAction::Ignore;
			}

			return config;
		}

	private:
		std::string m_config;
	};
}
