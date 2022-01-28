

#include "icsCoreTest.h"
#include "../core/ics_core.h"
#include <iostream>

namespace ics
{
	void testIcsCore()
	{
		Core control;

		ics_service::IcsSettings conf;
		conf.autostart = true;
		conf.enablePeriodScan = true;
		conf.period_scan = 5; // mins.
		conf.startTimeHH = 17;
		conf.startTimeMM = 30;

		control.applySettings(conf);

		while (true)
		{
			std::string cmd;
			std::cout << "Enter command (exit, change):";
			std::cin >> cmd;

			if (strings::equalStrings(cmd, "change"))
			{
				unsigned long hour, min;
				std::cout << "Hour:";
				std::cin >> hour;
				std::cout << "\nMin:";
				std::cin >> min;

				conf.startTimeHH = hour;
				conf.startTimeMM = min;
				control.applySettings(conf);
			}
			else if (strings::equalStrings(cmd, "exit"))
			{
				exit(0);
			}

			std::cout << "\n\n";
		}
	}

	unsigned long calculateTimeoutValue(unsigned long _hours, unsigned long _minutes,
		unsigned long _currentHour, unsigned long _currentMinutes)
	{
		unsigned long waitMinutes{};

		if (_currentHour <= _hours && _currentMinutes <= _minutes)
		{
			waitMinutes = (_hours - _currentHour) * 60 + abs(long(_minutes) - long(_currentMinutes));
		}
		else
		{
			waitMinutes = (23 - abs(long(_currentHour) - long(_hours))) * 60 + (60 - _currentMinutes + _minutes);
		}

		return waitMinutes;
	}
}
