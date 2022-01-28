

#include "../../../stdafx.h"
#include "icsDirectories.h"
#include "../../../helpers.h"
#include "../../../../../thrift/cpp/szi/SziService.h"
#include "../../../management/settings/sziLocalSettings.h"


namespace ics
{
	namespace directories
	{
		DirControl::DirControl(logfile& _log):
			m_log(_log),
			m_storage(SRV_STATE_A("ics_directories.szi"))
		{
			m_hashers[ics_service::HashAlg::Crc_32] = &m_hasherCrc32;
			m_hashers[ics_service::HashAlg::Md_5] = &m_hasherMd5;
			m_hashers[ics_service::HashAlg::Gost_94] = &m_hasherGost94;
			m_hashers[ics_service::HashAlg::Gost_2012_256] = &m_hasherGost2012_256;
			m_hashers[ics_service::HashAlg::Gost_2012_512] = &m_hasherGost2012_512;

			//m_hashers.insert(ics_service::HashAlg::Crc_32, directories::Hasher(m_hasherCrc32, m_log));
		}

		DirControl::~DirControl()
		{
			m_log.printEx("%s: _ITD_ directory control is destructing.", __FUNCTION__);

			m_storage.flush();
		}

		bool DirControl::isCompromised() const
		{
			DirsMap dirList;
			m_storage.toMap(dirList);

			for (auto dir : dirList)
			{
				if ((dir.second.status == STATE_CHANGED) || (dir.second.status == STATE_MISSED))
				{
					return true;
				}
			}

			return false;
		}

		bool DirControl::add(std::wstring _dirpath, bool _includeSubDirs, ics_service::HashAlg::type _algorithm, ics_service::IcsStatusCode::type& result)
		{
			result = ics_service::IcsStatusCode::type::Ics_Error;
			strings::toUpper(_dirpath);
			windir::removeLastSeparator(_dirpath);

			if (!windir::isFilePresent(_dirpath))
			{
				result = ics_service::IcsStatusCode::type::Ics_DirectoryNotFound;
				m_log.print(std::string(__FUNCTION__) + " failed because directory is not present: " + strings::ws_s(_dirpath));
				return false;
			}

			// Считаем контрольную сумму.
			auto hasher = m_hashers.find(_algorithm);
			if (hasher != m_hashers.end())
			{
				UINT64 totalSize = 0;
				std::string hash;
				if (directories::Hasher(*hasher->second, m_log).calculateDirHash(hash, _dirpath, _includeSubDirs, totalSize))
				{
					// Структура, та что летит в хранилище с описание контролируемого ключа.
					DirectoryDescription information = {0};
					information.status = STATE_NORMAL;
					information.version = 0;
					information.algorithm = _algorithm;
					information.includedDirs = _includeSubDirs;
					information.sizeInfo = totalSize;
					GetLocalTime(&information.time);

					fill_chars(information.hash, hash.c_str());
					fill_wchars(information.szDirPath, _dirpath.c_str());

					if (m_storage.add(information))
					{
						result = ics_service::IcsStatusCode::type::Ics_Success;
						m_log.print(std::string(__FUNCTION__) + " success - dir was added.");
						return true;
					}
					else
					{
						result = ics_service::IcsStatusCode::type::Ics_DirectoryIsExist;
						m_log.print(std::string(__FUNCTION__) + " error - couldn't add dir to control area, may be it was added earlier.");
					}
				}
				else
				{
					result = ics_service::IcsStatusCode::type::Ics_Error;
					m_log.print(std::string(__FUNCTION__) + " error - couldn't calculate hash.");
				}
			}
			else
			{
				result = ics_service::IcsStatusCode::type::Ics_UnknownAlgorithm;
				m_log.print(std::string(__FUNCTION__) + " error - unknown hashing algorithm was passed.");
			}

			return false;
		}

		bool DirControl::calc(const std::wstring& _dirpath,
		                      bool _includeSubdirs,
		                      ics_service::HashAlg::type _algType,
							  DirectoryDescription& _outInfo,
							  ics_service::IcsStatusCode::type& result)
		{
			result = ics_service::IcsStatusCode::type::Ics_Error;
			if (!windir::isFilePresent(_dirpath))
			{
				result = ics_service::IcsStatusCode::type::Ics_DirectoryNotFound;
				m_log.print(std::string(__FUNCTION__) + " failed because directory is not present: " + strings::ws_s(_dirpath));
				return false;
			}

			// Считаем контрольную сумму.
			auto hasher = m_hashers.find(_algType);
			if (hasher != m_hashers.end())
			{
				std::string hash;
				UINT64 totalSize = 0;
				if (directories::Hasher(*hasher->second, m_log).calculateDirHash(hash, _dirpath, _includeSubdirs, totalSize))
				{
					// Структура, та что летит в хранилище с описание контролируемого ключа.
					DirectoryDescription information = {0};
					information.status = STATE_NORMAL;
					information.version = 0;
					information.algorithm = _algType;
					information.includedDirs = _includeSubdirs;
					information.sizeInfo = totalSize;
					GetLocalTime(&information.time);

					fill_chars(information.hash, hash.c_str());
					fill_wchars(information.szDirPath, _dirpath.c_str());

					_outInfo = information;
					result = ics_service::IcsStatusCode::type::Ics_Success;
					return true;
				}
				else
				{
					//result = ics_service::IcsStatusCode::type::Ics_???;
					m_log.print(std::string(__FUNCTION__) + " error - couldn't calculate hash for the directorie.");
				}
			}
			else
			{
				result = ics_service::IcsStatusCode::type::Ics_UnknownAlgorithm;
				m_log.print(std::string(__FUNCTION__) + " error - unknown hashing algorithm was passed.");
			}

			return false;
		}

		bool DirControl::remove(std::wstring _dirpath)
		{
			strings::toUpper(_dirpath);
			windir::removeLastSeparator(_dirpath);

			if (m_storage.isPresent(_dirpath))
			{
				m_storage.remove(_dirpath);
				return true;
			}
			else
			{
				m_log.print(std::string(__FUNCTION__) + " error - object is not in a control area: " + strings::ws_s(_dirpath));
				return false;
			}
		}

		// Особеность поведения - при изменении(перерасчете хеша) алгоритма хеширования, целостность объекта 
		// считается восстановленной и он удаляется из списка изменённых ключей (если был там).
		bool DirControl::changeHashAlgorithm(std::wstring _dirpath, ics_service::HashAlg::type _hashAlg, ics_service::IcsStatusCode::type& result)
		{
			strings::toUpper(_dirpath);
			windir::removeLastSeparator(_dirpath);

			return hlpChangeHashAlgorithm(_dirpath, _hashAlg, result);
		}

		bool DirControl::hlpChangeHashAlgorithm(std::wstring _dirpath, ics_service::HashAlg::type _hashAlg, ics_service::IcsStatusCode::type& result)
		{
			result = ics_service::IcsStatusCode::type::Ics_Error;
			DirectoryDescription oldinfo = {0}, updated = {0};
			if (m_storage.find(_dirpath, oldinfo))
			{
				if (calc(_dirpath, oldinfo.includedDirs, _hashAlg, updated, result))
				{
					if (m_storage.update(_dirpath, updated))
					{
						result = ics_service::IcsStatusCode::type::Ics_Success;
						return true;
					}
					else
					{
						//result = ics_service::IcsStatusCode::type::Ics_???;
						m_log.print(std::string(__FUNCTION__) + " error - couldn't update information about : " + strings::ws_s(_dirpath));
					}
				}
				else
				{
					//result = ics_service::IcsStatusCode::type::Ics_???;
					m_log.print(std::string(__FUNCTION__) + " error - couldn't re-calculate checksum of : " + strings::ws_s(_dirpath));
				}
			}
			else
			{
				result = ics_service::IcsStatusCode::type::Ics_DirectoryNotFound;
				m_log.print(std::string(__FUNCTION__) + " error - object is not in a control area: " + strings::ws_s(_dirpath));
			}

			return false;
		}

		bool DirControl::recalculate(std::wstring _dirpath, bool& _isChanged, ics_service::IcsStatusCode::type& result)
		{
			result = ics_service::IcsStatusCode::type::Ics_Error;
			strings::toUpper(_dirpath);
			windir::removeLastSeparator(_dirpath);

			DirectoryDescription info = {0}, oldInfo;
			if (m_storage.find(_dirpath, oldInfo))
			{
				if (calc(_dirpath, oldInfo.includedDirs, static_cast<ics_service::HashAlg::type>(oldInfo.algorithm), info, result))
				{
					_isChanged = memcmp(oldInfo.hash, info.hash, sizeof(oldInfo.hash)) != 0;
					if (_isChanged)
					{
						markAsChanged(info);
					}
					else
					{
						markAsNormal(_dirpath);
					}

					result = ics_service::IcsStatusCode::type::Ics_Success;
					return true;
				}
				else
				{
					if (!windir::isFilePresent(_dirpath))
					{
						markAsMissed(_dirpath);
					}

					//result = ics_service::IcsStatusCode::type::Ics_???;
					m_log.print(std::string(__FUNCTION__) + " error - couldn't recalculate hash for: " + strings::ws_s(_dirpath));
				}
			}
			else
			{
				result = ics_service::IcsStatusCode::type::Ics_DirectoryNotFound;
				m_log.print(std::string(__FUNCTION__) + " error - object is not in a control area: " + strings::ws_s(_dirpath));
			}

			return false;
		}

		// Добавляет ключ в список изменённых, только если такой ключ имеется в списке контролируемых.
		void DirControl::markAsChanged(const DirectoryDescription& _dirpathInfo)
		{
			if (m_storage.setDirStatus(_dirpathInfo.szDirPath, STATE_CHANGED))
			{
				///// Отправка события безопасности
				szi::OperationResult result;
				result.errorCode = szi::SziStatusCodes::NoError;
				result.status = "NoError";
				getEventEmiter()->IntCntrlFileBrk("system", result, strings::ws_s(_dirpathInfo.szDirPath, CP_UTF8));
			}
		}

		bool DirControl::recalculate()
		{
			DirsMap dirList;
			m_storage.toMap(dirList);

			for (auto dir : dirList)
			{
				bool changed;
				ics_service::IcsStatusCode::type result;
				bool success = recalculate(dir.first, changed, result);
			}

			return true;
		}

		void DirControl::getList(DirsMap& _out) const
		{
			m_storage.toMap(_out);
		}

		void DirControl::getListChanged(DirsMap& _out) const
		{
			DirsMap dirList;
			m_storage.toMap(dirList);

			for (auto dir : dirList)
			{
				if (dir.second.status == STATE_CHANGED)
				{
					_out[dir.first] = dir.second;
				}
			}
		}

		bool DirControl::hasChangedObjects() const
		{
			DirsMap changed;
			getListChanged(changed);
			return !changed.empty();
		}

		void DirControl::markAsMissed(std::wstring _dir)
		{
			if (m_storage.setDirStatus(_dir, STATE_MISSED))
			{
				///// Отправка события безопасности
				szi::OperationResult result;
				result.errorCode = szi::SziStatusCodes::NoError;
				result.status = "NoError";
				getEventEmiter()->IntCntrlFileBrk("system", result, strings::ws_s(_dir, CP_UTF8));
			}
		}

		void DirControl::markAsNormal(std::wstring _dir)
		{
			m_storage.setDirStatus(_dir, STATE_NORMAL);
		}

		std::set<std::wstring> DirControl::getMissed() const
		{
			std::set<std::wstring> missed;
			DirsMap dirList;
			m_storage.toMap(dirList);

			for (auto dir : dirList)
			{
				if (dir.second.status == STATE_MISSED)
				{
					missed.insert(dir.first);
				}
			}

			return missed;
		}

		bool DirControl::isMissed(std::wstring _path) const
		{
			return getMissed().count(_path) != 0;
		}

		// Возвращает список доступных алгоритмов хэширования
		std::vector<ics_service::HashAlg::type> DirControl::getHashesList()
		{
			std::vector<ics_service::HashAlg::type> _list;
			std::string testString = "Lorem ipsum dolor sit amet";

			for (std::pair<ics_service::HashAlg::type, files::Hasher*> hashPair : m_hashers)
			{
				std::string hash = hashPair.second->getHash(testString.c_str(), testString.size());
				if (hash.size() > 0)
				{
					_list.push_back(hashPair.first);
				}
			}

			return _list;
		}

		std::shared_ptr<events_client::EventEmiter> DirControl::getEventEmiter()
		{
			return events_client::EventEmiterHolder::get("127.0.0.1", 
														 szi::Settings::get().getSziEventsLocalPort(m_log.getLogFilePath()),
														 m_log.getLogFilePath());
		}
	}
}
