

#pragma once

namespace ics
{
	void testIcsCore();

	unsigned long calculateTimeoutValue(unsigned long _hours, unsigned long _minutes,
		unsigned long _currentHour, unsigned long _currentMinutes);
}
