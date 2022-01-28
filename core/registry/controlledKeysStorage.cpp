

#include "controlledKeysStorage.h"

namespace ics
{
	namespace registry
	{
		bool Storage::add(const RegistryKeyDescription& _keyInfo)
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);

			std::wstring key = _keyInfo.szKeyPath;
			if(!m_keys.isPresent(RkdSelector(key)))
			{
				return m_keys.push_back(_keyInfo);
			}

			return false;
		}

		bool Storage::isPresent(const std::wstring& _regKey) const
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);
			return m_keys.isPresent(RkdSelector(_regKey));
		}

		bool Storage::find(const std::wstring& _regKey, RegistryKeyDescription& _outInfo) const
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);
			auto pos = m_keys.find_if(RkdSelector(_regKey));

			if (pos != RegKeysContainer::ElementNotFound)
			{
				_outInfo = m_keys.at(pos);
			}

			return pos != RegKeysContainer::ElementNotFound;
		}

		unsigned long Storage::toMap(KeyMap& _copyTo) const
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);

			RegKeysContainer::EntriesList keys;
			m_keys.toVector(keys);
			for (auto key : keys)
			{
				std::wstring keypath = key.szKeyPath;
				_copyTo[keypath] = key;
			}

			return keys.size();
		}

		bool Storage::update(const std::wstring& _regKey, const RegistryKeyDescription& _newInfo)
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);
			auto pos = m_keys.find_if(RkdSelector(_regKey));

			if (pos != RegKeysContainer::ElementNotFound)
			{
				m_keys.at(pos) = _newInfo;
			}

			return pos != RegKeysContainer::ElementNotFound;
		}

		bool Storage::setKeyStatus(const std::wstring& _regKey, ics::IntegrityState _state)
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);
			auto pos = m_keys.find_if(RkdSelector(_regKey));

			if (pos != RegKeysContainer::ElementNotFound)
			{
				m_keys.at(pos).status = _state;
			}

			return pos != RegKeysContainer::ElementNotFound;
		}

		bool Storage::getKeyStatus(const std::wstring& _regKey, ics::IntegrityState& _state) const
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);
			auto pos = m_keys.find_if(RkdSelector(_regKey));

			if (pos != RegKeysContainer::ElementNotFound)
			{
				_state = m_keys.at(pos).status;
			}

			return pos != RegKeysContainer::ElementNotFound;
		}

		void Storage::remove(const std::wstring& _regKey)
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);

			m_keys.remove_if(RkdSelector(_regKey));
		}

		void Storage::clear()
		{
			std::unique_lock<std::mutex> mtxlocker(m_lock);

			m_keys.clear();
		}
	}
}
