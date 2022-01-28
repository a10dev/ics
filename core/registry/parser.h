

#ifndef REGCONTROL_H
#define REGCONTROL_H

#include <stdio.h>
#include <map>
#include <set>
#include <string>
#include <list>
#include <vector>
#include <functional>
#define _WINSOCKAPI_
#include <windows.h>
#undef _WINSOCKAPI_
#include "../../../log.h"

namespace ics
{
	namespace registry
	{
		struct KeyValue
		{
			std::string name;
			std::string data;
			BYTE type;
		};

		struct WKeyValue
		{
			std::wstring name;     // name of registry value
			std::wstring regPath;  // path to this registry value
			std::vector<BYTE> data; // data field
			DWORD m_wow64Registry;  // Security and access rights: KEY_WOW64_32KEY, KEY_WOW64_64KEY, etc.
			BYTE type;
		};

		struct ValuesCompare
		{
			bool operator()(const KeyValue& _arg1, const KeyValue& _arg2)
			{
				return _arg1.name < _arg2.name;
			}
		};

		// Container for registry values.
		typedef std::map<std::string, KeyValue, std::greater<std::string>> KeyValues;


		typedef std::vector<WKeyValue> WKeyValues;

		class Parser
		{
		public:
			Parser(DWORD _useWow64 = 0, logfile* _log = nullptr);

			// Returns key values include default (unnamed) value in _keyValues param.
			//
			// An example of returned data:
			//
			// { "*HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\sudis", "", REG_SZ }, // default value
			// { "HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\sudis\cpu_type", "x86_64", REG_SZ}
			// { "HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\sudis\os_version", "WIN_7", REG_SZ}
			// 
			// Note:
			//	* at first position means unnamed default value.
			DWORD getKeyValues(const std::string& _key, KeyValues& _keyValues);

			DWORD getWKeyValues(const std::wstring& _key, WKeyValues& _keyValues);

			// Searches in sub-keys too and calls getKeyValues(..) for each value.
			DWORD getAllKeyValues(const std::string& _key, KeyValues& _keyValues);

			// 0 means success. An error has not occurred.
			int getLastError() const;

		protected:
			DWORD m_wow64Registry;
			logfile* m_log;
			int m_lastError;
		};

		// Provides handle of the _key which was opened with KEY_ALL_ACCESS rights.
		// 
		// An example of _key:
		//	HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\sudis
		int open(const std::string& _key,
		         HKEY& _result,
		         DWORD _additionalFlag = 0 /*KEY_WOW64_64KEY*/,
		         DWORD _accessRights = KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_READ);

		int wopen(const std::wstring& _key,
				 HKEY& _result,
				 DWORD _additionalFlag = 0 /*KEY_WOW64_64KEY*/, 
				 DWORD _accessRights = KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE | KEY_READ);
	}
}

#endif
