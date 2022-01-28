

#include "parser.h"
#include "../../../helpers.h"

#include <algorithm>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

namespace ics
{
	namespace registry
	{
		HKEY g_RegRootHandles[] = {
			HKEY_CLASSES_ROOT,
			HKEY_CURRENT_CONFIG,
			HKEY_CURRENT_USER,
			HKEY_LOCAL_MACHINE,
			HKEY_USERS
		};

		const char* g_RegRoot[] = {
			"HKEY_CLASSES_ROOT",
			"HKEY_CURRENT_CONFIG",
			"HKEY_CURRENT_USER",
			"HKEY_LOCAL_MACHINE",
			"HKEY_USERS"
		};

		const wchar_t* g_WRegRoot[] = {
			L"HKEY_CLASSES_ROOT",
			L"HKEY_CURRENT_CONFIG",
			L"HKEY_CURRENT_USER",
			L"HKEY_LOCAL_MACHINE",
			L"HKEY_USERS"
		};

		Parser::Parser(DWORD _useWow64, logfile* _log /* = nullptr */) :
			m_wow64Registry(_useWow64),
			m_log(_log),
			m_lastError(0)
		{
			// ...
		}

		int Parser::getLastError() const
		{
			return m_lastError;
		}

		DWORD Parser::getKeyValues(const std::string& _key, KeyValues& _keyValues)
		{
			HKEY hKey;
			DWORD dwNameSize, dwValueSize, dwValueType;
			bool defValueIsPresent = false;

			std::string regPath = _key;
			strings::toUpper(regPath);
			windir::removeLastSeparator(regPath);

			int status = open(regPath, hKey, m_wow64Registry);
			if (status != ERROR_SUCCESS)
			{
				if (m_log)
				{
					wsprintfA(m_log->acquireBuffer(), "Error %d. Couldn't open key %s", status, regPath.c_str());
					m_log->leaveBuffer();
				}

				m_lastError = status;
				return status;
			}

			DWORD cSubKeys = 0; // number of subkeys 
			DWORD cbMaxSubKey; // longest subkey size 
			DWORD cValues; // number of values for key 
			DWORD cchMaxValue; // longest value name 
			DWORD cbMaxValueData; // longest value data 
			DWORD retCode = RegQueryInfoKeyA(
				hKey, // key handle 
				NULL, // buffer for class name 
				NULL, // size of class string (without null character)
				NULL, // reserved 
				&cSubKeys, // number of subkeys
				&cbMaxSubKey, // longest subkey size (without null character)
				NULL, // longest class string (without null character)
				&cValues, // number of values for this key 
				&cchMaxValue, // longest value name (without null character)
				&cbMaxValueData, // longest value data 
				NULL, // security descriptor 
				NULL); // last write time 

			if (retCode != ERROR_SUCCESS)
			{
				if (m_log)
				{
					wsprintfA(m_log->acquireBuffer(), "Error %d. RegQueryInfoKey failed for %s", retCode, regPath.c_str());
					m_log->leaveBuffer();
				}
				RegCloseKey(hKey);
				m_lastError = retCode;
				return retCode;
			}

			// Reserve memory for last null terminating symbol.
			cchMaxValue += sizeof(char);
			char* pszName = (char*)memory::getmem(cchMaxValue);
			BYTE* pValueData = (BYTE*)memory::getmem(cbMaxValueData);

			if (!pszName || !pValueData)
			{
				if (m_log)
				{
					wsprintfA(m_log->acquireBuffer(),
					          "Error ERROR_NOT_ENOUGH_MEMORY. pszName=0x%x (%d bytes) pValueData=0x%x (%d bytes)",
					          pszName, cchMaxValue, pValueData, cbMaxValueData);
					m_log->leaveBuffer();
				}

				memory::freemem(pszName);
				memory::freemem(pValueData);
				m_lastError = ERROR_NOT_ENOUGH_MEMORY ;
				return ERROR_NOT_ENOUGH_MEMORY ;
			}

			for (DWORD i = 0, retCode = ERROR_SUCCESS; i < cValues; i++)
			{
				pszName[0] = '\0';
				dwNameSize = cchMaxValue;
				dwValueSize = cbMaxValueData;
				retCode = RegEnumValueA(hKey, i, pszName, &dwNameSize, NULL, &dwValueType, pValueData, &dwValueSize);
				if (retCode == ERROR_SUCCESS)
				{
					std::string name((const char*)pszName, dwNameSize);
					std::string data((const char*)pValueData, dwValueSize);

					if (name.empty())
					{
						defValueIsPresent = true;
						name = '*' + regPath;
					}
					else
					{
						name = regPath + '\\' + name;
					}

					KeyValue value;
					value.name.swap(name);
					value.data.swap(data);
					value.type = dwValueType;

					_keyValues[value.name] = value; // insert new value in a tree.
					//_keyValues.insert(std::pair<std::string, registry::KeyValue>(name, value)); // It works slowlier than operator[].
				}
				else
				{
					m_lastError = retCode;
					if (m_log)
					{
						wsprintfA(m_log->acquireBuffer(), "Error %d. RegEnumValue failed for %s : %s", retCode, regPath.c_str(), pszName);
						m_log->leaveBuffer();
					}
				}
			}

			// If it's a key which has only a default empty value.
			if (defValueIsPresent == false)
			{
				KeyValue value;
				value.type = REG_SZ ;
				value.name = '*' + regPath;
				_keyValues[value.name] = value; // insert new value in a tree.
			}

			memory::freemem(pValueData);
			memory::freemem(pszName);
			RegCloseKey(hKey);
			return retCode;
		}


		DWORD Parser::getWKeyValues(const std::wstring& _key, WKeyValues& _keyValues)
		{
			HKEY hKey;

			std::wstring regPath = _key;
			strings::toUpper(regPath);
			windir::removeLastSeparator(regPath);

			int status = wopen(regPath, hKey, m_wow64Registry);
			if (status != ERROR_SUCCESS)
			{
				if (m_log)
				{
					wsprintfA(m_log->acquireBuffer(), "Error %d. Couldn't open key %s", status, strings::ws_s(regPath, CP_UTF8).c_str());
					m_log->leaveBuffer();
				}

				m_lastError = status;
				return status;
			}

			DWORD cSubKeys = 0; // number of subkeys 
			DWORD cbMaxSubKey; // longest subkey size 
			DWORD cValues; // number of values for key 
			DWORD cchMaxValue; // longest value name 
			DWORD cbMaxValueData; // longest value data 
			DWORD retCode = RegQueryInfoKeyW(
				hKey, // key handle 
				NULL, // buffer for class name 
				NULL, // size of class string (without null character)
				NULL, // reserved 
				&cSubKeys, // number of subkeys
				&cbMaxSubKey, // longest subkey size (without null character)
				NULL, // longest class string (without null character)
				&cValues, // number of values for this key 
				&cchMaxValue, // longest value name (without null character)
				&cbMaxValueData, // longest value data 
				NULL, // security descriptor 
				NULL); // last write time 

			if (retCode != ERROR_SUCCESS)
			{
				if (m_log)
				{
					wsprintfA(m_log->acquireBuffer(), "Error %d. RegQueryInfoKey failed for %s", retCode, strings::ws_s(regPath, CP_UTF8).c_str());
					m_log->leaveBuffer();
				}
				RegCloseKey(hKey);
				m_lastError = retCode;
				return retCode;
			}

			try
			{
				cchMaxValue++;				// plus null terminator
				std::vector<wchar_t> pszName(cchMaxValue);    // input
				std::vector<BYTE> pValueData(cbMaxValueData); // buffers

				for (DWORD i = 0; i < cValues; i++)
				{
					DWORD dwValueType;
					auto dwNameSize = cchMaxValue;
					auto dwValueSize = cbMaxValueData;
					retCode = RegEnumValueW(hKey, i, pszName.data(), &dwNameSize, NULL, &dwValueType, pValueData.data(), &dwValueSize);
					if (retCode == ERROR_SUCCESS)
					{
                        std::wstring name(pszName.data(), dwNameSize);
                        //std::wstring name(pszName.begin(), pszName.begin() + dwNameSize);
						if (!name.empty()) // if value exists
						{
							std::vector<BYTE> data(pValueData.begin(), pValueData.begin() + dwValueSize);

							WKeyValue value;
							value.name.swap(name);
							value.data.swap(data);
							value.regPath.assign(regPath);
							value.type = dwValueType;
							value.m_wow64Registry = m_wow64Registry;

							_keyValues.push_back(value); // insert new value in a tree.
						}
					}
					else
					{
						m_lastError = retCode;
						if (m_log)
						{
							wsprintfA(m_log->acquireBuffer(), "Error %d. RegEnumValue failed for %s", retCode, strings::ws_s(regPath, CP_UTF8).c_str());
							m_log->leaveBuffer();
						}
					}
				}

				RegCloseKey(hKey);
				return retCode;
			}
			catch (std::exception& e)
			{
				if (m_log)
				{
					wsprintfA(m_log->acquireBuffer(), "Error: %s", e.what());
					m_log->leaveBuffer();
				}

				m_lastError = ERROR_NOT_ENOUGH_MEMORY;
				return ERROR_NOT_ENOUGH_MEMORY;
			}
		}


		DWORD Parser::getAllKeyValues(const std::string& _key, KeyValues& _keyValues)
		{
			HKEY hKey;
			std::string regPath = _key;
			strings::toUpper(regPath);
			windir::removeLastSeparator(regPath);
			int status = open(regPath, hKey, m_wow64Registry);
			if (status != ERROR_SUCCESS)
			{
				if (m_log)
				{
					wsprintfA(m_log->acquireBuffer(), "Error %d. Couldn't open key %s", status, regPath.c_str());
					m_log->leaveBuffer();
				}
				m_lastError = status;
				return status;
			}

			DWORD dwSubKeyNameSize;
			DWORD cSubKeys = 0; // number of subkeys 
			DWORD cbMaxSubKey; // longest subkey size 
			DWORD retCode = RegQueryInfoKeyA(
				hKey, // key handle 
				NULL, // buffer for class name 
				NULL, // size of class string (without null character)
				NULL, // reserved 
				&cSubKeys, // number of subkeys
				&cbMaxSubKey, // longest subkey size (without null character)
				nullptr, // longest class string (without null character)
				NULL, // number of values for this key 
				NULL, // longest value name (without null character)
				NULL, // longest value data 
				NULL, // security descriptor 
				NULL); // last write time 

			if (retCode != ERROR_SUCCESS)
			{
				if (m_log)
				{
					wsprintfA(m_log->acquireBuffer(), "RegQueryInfoKey failed for %s with error %d", regPath.c_str(), retCode);
					m_log->leaveBuffer();
				}
				m_lastError = retCode;
				RegCloseKey(hKey);
				return retCode;
			}

			// Read all values of the current key.
			retCode = getKeyValues(regPath, _keyValues);
			if (retCode != ERROR_SUCCESS)
			{
				if (m_log)
				{
					wsprintfA(m_log->acquireBuffer(), "failed getKeyValues for %s with error %d", regPath.c_str(), retCode);
					m_log->leaveBuffer();
				}
				m_lastError = retCode;
				RegCloseKey(hKey);
				return retCode;
			}

			// Reserve memory for last null terminating symbol.
			cbMaxSubKey += sizeof(char);
			char* pszSubKeyName = (char*)memory::getmem(cbMaxSubKey);
			if (!pszSubKeyName)
			{
				if (m_log)
				{
					wsprintfA(m_log->acquireBuffer(), "failed memory::getmem( %d bytes)", cbMaxSubKey);
					m_log->leaveBuffer();
				}
				RegCloseKey(hKey);
				memory::freemem(pszSubKeyName);
				m_lastError = ERROR_NOT_ENOUGH_MEMORY ;
				return ERROR_NOT_ENOUGH_MEMORY ;
			}

			for (DWORD i = 0; i < cSubKeys; i++)
			{
				pszSubKeyName[0] = '\0';
				dwSubKeyNameSize = cbMaxSubKey;
				retCode = RegEnumKeyExA(hKey, i, pszSubKeyName, &dwSubKeyNameSize, NULL, NULL, NULL, NULL);
				if (retCode == ERROR_SUCCESS)
				{
					getAllKeyValues(regPath + '\\' + pszSubKeyName, _keyValues);
				}
				else
				{
					m_lastError = retCode;
					if (m_log)
					{
						wsprintfA(m_log->acquireBuffer(), "Error %d. RegEnumKeyEx failed for %s : %s", retCode, regPath.c_str(), pszSubKeyName);
						m_log->leaveBuffer();
					}
				}
			}

			RegCloseKey(hKey);
			memory::freemem(pszSubKeyName);
			return ERROR_SUCCESS;
		}

		std::string getKeyPathWithoutRootDir(const std::string& _key, HKEY& _hRootDir)
		{
			std::string result;
			std::size_t pos = std::string::npos;
			int i = 0;

			for (i = 0; i < sizeof(g_RegRoot) / sizeof(const char*); ++i)
			{
				pos = _key.find(g_RegRoot[i]);
				if (pos != std::string::npos)
				{
					break;
				}
			}

			if (pos != std::string::npos)
			{
				if (std::string(g_RegRoot[i]).length() + 1 < _key.length())
				{
					result = _key.substr(std::string(g_RegRoot[i]).length() + 1 /* \ */);
				}
				else
				{
					result = "";
				}

				_hRootDir = g_RegRootHandles[i];
			}

			return result;
		}

		std::wstring getWKeyPathWithoutRootDir(const std::wstring& _key, HKEY& _hRootDir)
		{
			std::wstring result;
			std::size_t pos = std::wstring::npos;
			int i = 0;

			for (i = 0; i < sizeof(g_WRegRoot) / sizeof(const wchar_t*); ++i)
			{
				pos = _key.find(g_WRegRoot[i]);
				if (pos != std::wstring::npos)
				{
					if (std::wstring(g_WRegRoot[i]).length() + 1 < _key.length())
					{
						result = _key.substr(std::wstring(g_WRegRoot[i]).length() + 1);
					}
					else
					{
						result = L"";
					}

					_hRootDir = g_RegRootHandles[i];

					break;
				}
			}

			return result;
		}

		int open(const std::string& _key, HKEY& _result, DWORD _flag, DWORD _accessRights)
		{
			HKEY rootKey = 0;
			std::string keyPath = getKeyPathWithoutRootDir(_key, rootKey);

			int errorCode = RegOpenKeyExA(rootKey,
			                              keyPath.c_str(),
			                              0,
			                              _accessRights | _flag,
			                              &_result);

			if (errorCode != ERROR_SUCCESS)
			{
				return errorCode;
			}

			return ERROR_SUCCESS;
		}

		int wopen(const std::wstring& _key, HKEY& _result, DWORD _flag, DWORD _accessRights)
		{
			HKEY rootKey = 0;
			std::wstring keyPath = getWKeyPathWithoutRootDir(_key, rootKey);

			int errorCode = RegOpenKeyExW(rootKey,
				keyPath.c_str(),
				0,
				_accessRights | _flag,
				&_result);

			if (errorCode != ERROR_SUCCESS)
			{
				return errorCode;
			}

			return ERROR_SUCCESS;
		}
	}
}
