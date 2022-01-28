

#pragma once

#include "../../../tstorage.h"
#include "registryKeyDescription.h"
#include <map>
#include "../../../helpers.h"
#include <boost/noncopyable.hpp>

namespace ics
{
	namespace registry
	{
		class RkdSelector // Registry key description selector.
		{
		public:
			RkdSelector(std::wstring _keypath) :m_keypath(_keypath)
			{
				strings::toUpper(m_keypath);
			}

			bool operator()(const RegistryKeyDescription& _entry) const
			{
				return strings::equalStrings(m_keypath, _entry.szKeyPath);
			}

		private:
			std::wstring m_keypath; // Upper-case string.
		};

		typedef TStorage<ics::registry::RegistryKeyDescription, RkdSelector> RegKeysContainer;
		typedef std::map<std::wstring, RegistryKeyDescription> KeyMap;


		class Storage : private boost::noncopyable
		{
		public:

			Storage(std::string _keysStoragePath /*= "ics_regkeys.szi"*/) :m_keys(_keysStoragePath){}

			// Adds new entry, faild when key was added earlier.
			bool add(const RegistryKeyDescription& _keyInfo);

			// Returns true when storage has information about key.
			bool isPresent(const std::wstring& _regKey) const;

			// Returns information about early added key to control area.
			bool find(const std::wstring& _regKey, RegistryKeyDescription& _outInfo) const;

			// Returns information about all controlled keys in map-like view.
			unsigned long toMap(KeyMap& _copyTo) const;

			// Updates information about early added key, fails if has no info about key in the storage.
			bool update(const std::wstring& _regKey, const RegistryKeyDescription& _newInfo);

			// Changes state of integrity to the specific registry key.
			bool setKeyStatus(const std::wstring& _regKey, ics::IntegrityState _state);

			bool getKeyStatus(const std::wstring& _regKey, ics::IntegrityState& _state) const;

			// Remove info about key.
			void remove(const std::wstring& _regKey);

			// Removes all information.
			void clear();

			void flush()
			{
				std::unique_lock<std::mutex> mtxlocker(m_lock);

				m_keys.flush();
			}

		private:
			RegKeysContainer m_keys;
			mutable std::mutex m_lock;
		};
	}
}
