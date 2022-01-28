

#include "../../../stdafx.h"
#include "icsRegistry.h"
#include "../../../helpers.h"
#include "../../../../../thrift/cpp/szi/sziTypes_types.h"
#include "../../../management/settings/sziLocalSettings.h"

namespace ics
{
	namespace registry
	{
		RegControl::RegControl(logfile& _log):
			m_log(_log),
			m_storage(SRV_STATE_A("ics_registry.szi"))
		{
			m_hashers[ics_service::HashAlg::Crc_32] = &m_hasherCrc32;
			m_hashers[ics_service::HashAlg::Md_5] = &m_hasherMd5;
			m_hashers[ics_service::HashAlg::Gost_94] = &m_hasherGost94;
			m_hashers[ics_service::HashAlg::Gost_2012_256] = &m_hasherGost2012_256;
			m_hashers[ics_service::HashAlg::Gost_2012_512] = &m_hasherGost2012_512;
		}

		RegControl::~RegControl()
		{
			m_log.printEx("%s: _ITD_ registry control is destructing.", __FUNCTION__);

			m_storage.flush();
		}

		bool RegControl::isCompromised() const
		{
			KeyMap keys;
			m_storage.toMap(keys);
			for (auto key : keys){
				if ((key.second.status == STATE_CHANGED) || (key.second.status == STATE_MISSED)){
					return true;
				}
			}

			return false;
		}

		bool RegControl::addKey(std::wstring _keypath, bool _includeSubkeys, ics_service::HashAlg::type _hashAlgorithm, ics_service::IcsStatusCode::type& result)
		{
			result = ics_service::IcsStatusCode::type::Ics_Error;
			strings::toUpper(_keypath);
			windir::removeLastSeparator(_keypath);
			if (::registry::isKeyExist(_keypath) == false)
			{
				result = ics_service::IcsStatusCode::type::Ics_KeyNotFound;
				m_log.print(std::string(__FUNCTION__) + " error - key is not exist " + strings::ws_s(_keypath));
				return false;
			}

			KeyValues tree;
			Parser parser(KEY_WOW64_64KEY, &m_log);

			// Строим список всех ключей и подключей для вычисления итоговой контрольной суммы.
			bool buildKeyTreeState = (0 == (_includeSubkeys ? parser.getAllKeyValues(strings::ws_s(_keypath), tree) : parser.getKeyValues(strings::ws_s(_keypath), tree)));

			// Считаем контрольную сумму.
			auto hasher = m_hashers.find(_hashAlgorithm);
			if (hasher != m_hashers.end())
			{
				std::string hash = hasher->second->calculateKey(tree);
				if (!hash.empty())
				{
					// Структура, та что летит в хранилище с описание контролируемого ключа.
					RegistryKeyDescription controlledKeyInfo = { 0 };

					controlledKeyInfo.status = STATE_NORMAL;
					controlledKeyInfo.version = 0;
					controlledKeyInfo.algorithm = _hashAlgorithm;
					controlledKeyInfo.subKeysIncluded = _includeSubkeys;
					GetLocalTime(&controlledKeyInfo.time);
					controlledKeyInfo.countObjects = tree.size();

					fill_chars(controlledKeyInfo.hash, hash.c_str());
					fill_wchars(controlledKeyInfo.szKeyPath, _keypath.c_str());

					if(m_storage.add(controlledKeyInfo))
					{
						result = ics_service::IcsStatusCode::type::Ics_Success;
						m_log.print(std::string(__FUNCTION__) + " success - key was added.");
						return true;
					} else
					{
						result = ics_service::IcsStatusCode::type::Ics_RegistryKeyIsExist;
						m_log.print(std::string(__FUNCTION__) + " error - couldn't add key to control area, may be key was added there earlier.");
					}
				}
				else
				{
					result = ics_service::IcsStatusCode::type::Ics_Error;
					m_log.print(std::string(__FUNCTION__) + " error - couldn't calculate hash of just built registry tree.");
				}
			}
			else
			{
				result = ics_service::IcsStatusCode::type::Ics_UnknownAlgorithm;
				m_log.print(std::string(__FUNCTION__) + " error - unknown hashing algorithm was passed.");
			}

			return false;
		}

		bool RegControl::calc(const std::wstring& _keypath,
			bool _includeSubkeys,
			ics_service::HashAlg::type _algType,
			RegistryKeyDescription& _outInfo,
			ics_service::IcsStatusCode::type& result)
		{
			result = ics_service::IcsStatusCode::type::Ics_Error;
			if (! ::registry::isKeyExist(_keypath))
			{ // Ничего не делать, если не удается получить доступ к ключу.
				result = ics_service::IcsStatusCode::type::Ics_KeyNotFound;
				return false;
			}

			KeyValues tree;
			Parser parser(KEY_WOW64_64KEY, &m_log);
			auto resultCode = _includeSubkeys ? parser.getAllKeyValues(strings::ws_s(_keypath), tree) : parser.getKeyValues(strings::ws_s(_keypath), tree);

			// Строим список всех ключей и подключей для вычисления итоговой контрольной суммы.
			bool buildKeyTreeState = (0 == resultCode);

			// Считаем контрольную сумму.
			auto hasher = m_hashers.find(_algType);
			if (hasher != m_hashers.end())
			{
				std::string hash = hasher->second->calculateKey(tree);
				if (!hash.empty())
				{
					// Структура, та что летит в хранилище с описание контролируемого ключа.
					RegistryKeyDescription controlledKeyInfo = { 0 };

					controlledKeyInfo.status = STATE_NORMAL;
					controlledKeyInfo.version = 0;
					controlledKeyInfo.algorithm = _algType;
					controlledKeyInfo.subKeysIncluded = _includeSubkeys;
					GetLocalTime(&controlledKeyInfo.time);
					controlledKeyInfo.countObjects = tree.size();

					fill_chars(controlledKeyInfo.hash, hash.c_str());
					fill_wchars(controlledKeyInfo.szKeyPath, _keypath.c_str());

					_outInfo = controlledKeyInfo;
					result = ics_service::IcsStatusCode::type::Ics_Success;
					return true;
				}
				else
				{
					//result = ics_service::IcsStatusCode::type::Ics_???;
					m_log.print(std::string(__FUNCTION__) + " error - couldn't calculate hash of just built registry tree.");
				}
			}
			else
			{
				result = ics_service::IcsStatusCode::type::Ics_UnknownAlgorithm;
				m_log.print(std::string(__FUNCTION__) + " error - unknown hashing algorithm was passed.");
			}

			return false;
		}

		bool RegControl::removeKey(std::wstring _keypath)
		{
			strings::toUpper(_keypath);
			windir::removeLastSeparator(_keypath);

			if (m_storage.isPresent(_keypath))
			{
				m_storage.remove(_keypath);
				return true;
			}
			else
			{
				m_log.print(std::string(__FUNCTION__) + " error - key is not in a control area: " + strings::ws_s(_keypath));
				return false;
			}
		}

		// Особеность поведения - при изменении(перерасчете хеша) алгоритма хеширования, целостность объекта 
		// считается восстановленной и он удаляется из списка изменённых ключей (если был там).
		bool RegControl::changeHashAlgorithm(std::wstring _keypath, ics_service::HashAlg::type _hashAlg, ics_service::IcsStatusCode::type& result)
		{
			strings::toUpper(_keypath);

			bool changed = hlpChangeHashAlgorithm(_keypath, _hashAlg, result);
			if (changed)
			{
				markAsNormal(_keypath);
			}

			return changed;
		}

		bool RegControl::hlpChangeHashAlgorithm(std::wstring _keypath, ics_service::HashAlg::type _hashAlg, ics_service::IcsStatusCode::type& result)
		{
			result = ics_service::IcsStatusCode::type::Ics_Error;
			RegistryKeyDescription oldinfo = { 0 }, updated = { 0 };
			if (m_storage.find(_keypath, oldinfo))
			{
				if (calc(_keypath, oldinfo.subKeysIncluded, _hashAlg, updated, result))
				{
					if (m_storage.update(_keypath, updated))
					{
						result = ics_service::IcsStatusCode::type::Ics_Success;
						return true;
					}
					else
					{
						// result = ics_service::IcsStatusCode::type::Ics_???;
						m_log.print(std::string(__FUNCTION__) + " error - couldn't update information about the key: " + strings::ws_s(_keypath));
					}
				}
				else
				{
					// result = ics_service::IcsStatusCode::type::Ics_???;
					m_log.print(std::string(__FUNCTION__) + " error - couldn't re-calculate checksum of the key: " + strings::ws_s(_keypath));
				}
			}
			else
			{
				result = ics_service::IcsStatusCode::type::Ics_KeyNotFound;
				m_log.print(std::string(__FUNCTION__) + " error - key is not in a control area: " + strings::ws_s(_keypath));
			}

			return false;
		}

		bool RegControl::recalculate(std::wstring _keypath, bool& _isChanged, ics_service::IcsStatusCode::type& result)
		{
			result = ics_service::IcsStatusCode::type::Ics_Error;
			strings::toUpper(_keypath);
			windir::removeLastSeparator(_keypath);

			RegistryKeyDescription info = { 0 }, oldInfo = { 0 };
			if (m_storage.find(_keypath, oldInfo))
			{
				if (calc(_keypath, oldInfo.subKeysIncluded, static_cast<ics_service::HashAlg::type>(oldInfo.algorithm), info, result))
				{
					_isChanged = memcmp(oldInfo.hash, info.hash, sizeof(oldInfo.hash)) != 0;
					if (_isChanged) // Если целостность была нарушена, добавить его в список изменённых.
					{
						markAsChanged(info);
					}
					else
					{
						markAsNormal(_keypath); // Удалить в случае если целостность не нарушена.
					}

					result = ics_service::IcsStatusCode::type::Ics_Success;
					return true;
				}
				else
				{
					if (::registry::isKeyExist(_keypath) == false)
					{
						markAsMissed(_keypath);
					}

					//result = ics_service::IcsStatusCode::type::Ics_???;
					m_log.print(std::string(__FUNCTION__) + " error - couldn't recalculate hash for: " + strings::ws_s(_keypath));
				}
			} else
			{
				result = ics_service::IcsStatusCode::type::Ics_KeyNotFound;
				m_log.print(std::string(__FUNCTION__) + " error - key is not in a control area: " + strings::ws_s(_keypath));
			}

			return false;
		}



		// Тут нужно быть осторожным, в следующей ситуации - 
		// Во время перерасчёта некоторый ключи могут быть удалены из области контроля целостности, 
		// а затем могут быть добавлены снова после перерасчета.
		bool RegControl::recalculate()
		{
			KeyMap list;
			m_storage.toMap(list);

			for (auto key : list)
			{
				bool changed;
				ics_service::IcsStatusCode::type result;
				bool success = recalculate(key.first, changed, result);
			}

			return true;
		}

		void RegControl::getListKeys(KeyMap& _outKeys) const
		{
			m_storage.toMap(_outKeys);
		}

		void RegControl::getListChangedKeys(KeyMap& _outKeys) const
		{
			KeyMap keys;
			m_storage.toMap(keys);

			for (auto key :keys)
			{
				if (key.second.status == STATE_CHANGED)
				{
					_outKeys[key.first] = key.second;
				}
			}
		}
		
		bool RegControl::hasChangedObjects() const
		{
			KeyMap changedKeys;
			getListChangedKeys(changedKeys);

			return !changedKeys.empty();
		}

		StrSet RegControl::getMissedKeys() const
		{
			StrSet missedKeys;
			
			KeyMap keys;
			m_storage.toMap(keys);
			for (auto key : keys)
			{
				if (key.second.status == STATE_MISSED)
				{
					missedKeys.insert(key.first);
				}
			}

			return missedKeys;
		}

		// Добавляет ключ в список изменённых, только если такой ключ имеется в списке контролируемых.
		void RegControl::markAsChanged(RegistryKeyDescription _keyInfo)
		{
			if (m_storage.setKeyStatus(_keyInfo.szKeyPath, STATE_CHANGED))
			{
				///// Отправка события безопасности
				szi::OperationResult result;
				result.errorCode = szi::SziStatusCodes::NoError;
				result.status = "NoError";
				getEventEmiter()->IntCntrlRegBrk("system", result, strings::ws_s(_keyInfo.szKeyPath, CP_UTF8));
			}
		}

		void RegControl::markAsNormal(std::wstring _key)
		{
			strings::toUpper(_key);

			// Updates info in a local storage.
			m_storage.setKeyStatus(_key, STATE_NORMAL);
		}

		void RegControl::markAsMissed(std::wstring _key)
		{
			m_storage.setKeyStatus(_key, STATE_MISSED);

		///// Отправка события безопасности
			szi::OperationResult result;
			result.errorCode = szi::SziStatusCodes::NoError;
			result.status = "NoError";
			getEventEmiter()->IntCntrlRegBrk("system", result, strings::ws_s(_key, CP_UTF8));
		}

		// Возвращает список доступных алгоритмов хэширования
		std::vector<ics_service::HashAlg::type> RegControl::getHashesList()
		{
			std::vector<ics_service::HashAlg::type> _list;
			std::string testString = "Lorem ipsum dolor sit amet";

			for (std::pair<ics_service::HashAlg::type, Hasher*> hashPair : m_hashers)
			{
				std::string hash = hashPair.second->getHash(testString.c_str(), testString.size());
				if (hash.size() > 0)
				{
					_list.push_back(hashPair.first);
				}
			}

			return _list;
		}

		std::shared_ptr<events_client::EventEmiter> RegControl::getEventEmiter()
		{
			return events_client::EventEmiterHolder::get("127.0.0.1", 
														 szi::Settings::get().getSziEventsLocalPort(m_log.getLogFilePath()),
														 m_log.getLogFilePath());
		}
	}
}
