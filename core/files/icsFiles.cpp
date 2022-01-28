

#include "../../../stdafx.h"
#include "icsFiles.h"
#include "../../../helpers.h"
#include "../../../../../thrift/cpp/szi/SziService.h"
#include "../../../arm_events_client/event_emiter.h"
#include "../../../management/settings/sziLocalSettings.h"


namespace ics
{
	namespace files
	{
		FilesControl::FilesControl(logfile& _log):
			m_log(_log),
			m_storage(SRV_STATE_A("ics_files.szi"))
		{
			m_hashers[ics_service::HashAlg::Crc_32] = &m_hasherCrc32;
			m_hashers[ics_service::HashAlg::Md_5] = &m_hasherMd5;
			m_hashers[ics_service::HashAlg::Gost_94] = &m_hasherGost94;
			m_hashers[ics_service::HashAlg::Gost_2012_256] = &m_hasherGost2012_256;
			m_hashers[ics_service::HashAlg::Gost_2012_512] = &m_hasherGost2012_512;
		}

		FilesControl::~FilesControl()
		{
			m_log.printEx("%s: _ITD_ files control is destructing.", __FUNCTION__);

			m_storage.flush();
		}

		bool FilesControl::isCompromised() const
		{
			FilesMap files;
			m_storage.toMap(files);

			for (auto f : files){
				if ((f.second.status == STATE_CHANGED) || (f.second.status == STATE_MISSED)){
					return true;
				}
			}

			return false;
		}

		bool FilesControl::add(std::wstring _file, ics_service::HashAlg::type _algorithm, ics_service::IcsStatusCode::type& result)
		{
			result = ics_service::IcsStatusCode::type::Ics_Error;
			strings::toUpper(_file);
			if (!windir::isFilePresent(_file))
			{
				result = ics_service::IcsStatusCode::type::Ics_FileNotFound;
				m_log.print(std::string(__FUNCTION__) + " failed because file is not present: " + strings::ws_s(_file));
				return false;
			}

			// !! хэш посчитать то надо как-то!

			// Считаем контрольную сумму.
			auto hasher = m_hashers.find(_algorithm);
			if (hasher != m_hashers.end())
			{
				std::string hash;
				auto hashResult = hasher->second->calculateFileHash(_file, hash);
				if (hashResult == NoError)
				{
					// Структура, та что летит в хранилище с описание контролируемого ключа.
					FileDescription information = { 0 };
					information.version = 0;
					information.algorithm = _algorithm;
					information.status = STATE_NORMAL;
					GetLocalTime(&information.time);

					// Calculate file size.
					windir::getFileByteSize(_file, (size_t&)information.fileSize);

					fill_chars(information.hash, hash.c_str());
					fill_wchars(information.szFilePath, _file.c_str());

					if(m_storage.add(information))
					{
						result = ics_service::IcsStatusCode::type::Ics_Success;
						m_log.print(std::string(__FUNCTION__) + " success - file was added.");
						return true;
					} else
					{
						result = ics_service::IcsStatusCode::type::Ics_FileIsExist;
						m_log.print(std::string(__FUNCTION__) + " error - couldn't add file to control area, may be it was added earlier.");
					}
				}
				else
				{
					result = ics_service::IcsStatusCode::type::Ics_Error;
					m_log.print(std::string(__FUNCTION__) + " error - couldn't calculate hash of the file.");
				}
			}
			else
			{
				result = ics_service::IcsStatusCode::type::Ics_UnknownAlgorithm;
				m_log.print(std::string(__FUNCTION__) + " error - unknown hashing algorithm was passed.");
			}

			return false;
		}

		bool FilesControl::calc(const std::wstring& _filepath,
			ics_service::HashAlg::type _algType,
			FileDescription& _outInfo,
			ics_service::IcsStatusCode::type& result)
		{
			result = ics_service::IcsStatusCode::type::Ics_Error;
			// Считаем контрольную сумму.
			auto hasher = m_hashers.find(_algType);
			if (hasher != m_hashers.end())
			{
				std::string hash;
				auto hashResult = hasher->second->calculateFileHash(_filepath, hash);
				if (hashResult == NoError)
				{
					// Структура, та что летит в хранилище с описание контролируемого ключа.
					FileDescription information = { 0 };
					information.status = STATE_NORMAL;
					information.version = 0;
					information.algorithm = _algType;
					GetLocalTime(&information.time);

					fill_chars(information.hash, hash.c_str());
					fill_wchars(information.szFilePath, _filepath.c_str());

					_outInfo = information;
					result = ics_service::IcsStatusCode::type::Ics_Success;
					return true;
				}
				else if (result == LockedAccess)
				{
					markAsLocked(_filepath);
				}
				else if (result == HasherErrors::Missed)
				{
					markAsMissed(_filepath);
				}
				else
				{
					//result = ics_service::IcsStatusCode::type::Ics_???;
					m_log.print(std::string(__FUNCTION__) + " error - couldn't calculate hash for the file.");
				}
			}
			else
			{
				result = ics_service::IcsStatusCode::type::Ics_UnknownAlgorithm;
				m_log.print(std::string(__FUNCTION__) + " error - unknown hashing algorithm was passed.");
			}

			return false;
		}

		bool FilesControl::remove(std::wstring _filepath)
		{
			if (m_storage.isPresent(_filepath))
			{
				m_storage.remove(_filepath);
				return true;
			}
			else
			{
				m_log.print(std::string(__FUNCTION__) + " error - object is not in a control area: " + strings::ws_s(_filepath));
				return false;
			}
		}

		// Особеность поведения - при изменении(перерасчете хеша) алгоритма хеширования, целостность объекта 
		// считается восстановленной.
		bool FilesControl::changeHashAlgorithm(std::wstring _filepath, ics_service::HashAlg::type _hashAlg, ics_service::IcsStatusCode::type& result)
		{
			strings::toUpper(_filepath);
			return hlpChangeHashAlgorithm(_filepath, _hashAlg, result);
		}

		bool FilesControl::hlpChangeHashAlgorithm(const std::wstring& _filepath, ics_service::HashAlg::type _hashAlg, ics_service::IcsStatusCode::type& result)
		{
			result = ics_service::IcsStatusCode::type::Ics_Error;
			FileDescription oldinfo = { 0 }, updated = { 0 };

			if (m_storage.find(_filepath, oldinfo))
			{
				if (calc(_filepath, _hashAlg, updated, result))
				{
					if (m_storage.update(_filepath, updated))
					{
						result = ics_service::IcsStatusCode::type::Ics_Success;
						return true;
					}
					else
					{
						//result = ics_service::IcsStatusCode::type::Ics_???;
						m_log.print(std::string(__FUNCTION__) + " error - couldn't update information about : " + strings::ws_s(_filepath));
					}
				}
				else
				{
					//result = ics_service::IcsStatusCode::type::Ics_???;
					m_log.print(std::string(__FUNCTION__) + " error - couldn't re-calculate checksum of : " + strings::ws_s(_filepath));
				}
			}
			else
			{
				result = ics_service::IcsStatusCode::type::Ics_FileNotFound;
				m_log.print(std::string(__FUNCTION__) + " error - object is not in a control area: " + strings::ws_s(_filepath));
			}

			return false;
		}

		bool FilesControl::recalculate(std::wstring _filepath, bool& _isChanged, ics_service::IcsStatusCode::type& result)
		{
			result = ics_service::IcsStatusCode::type::Ics_Error;
			strings::toUpper(_filepath);
			windir::removeLastSeparator(_filepath);

			FileDescription info = { 0 }, oldInfo = { 0 };
			if (m_storage.find(_filepath, oldInfo))
			{
				if (calc(_filepath, static_cast<ics_service::HashAlg::type>(oldInfo.algorithm), info, result))
				{
					_isChanged = memcmp(oldInfo.hash, info.hash, sizeof(oldInfo.hash)) != 0;

					// Если целостность была нарушена, добавить его в список изменённых.
					if (_isChanged)
					{
						markAsChanged(info);
					}
					else
					{
						markAsNormal(_filepath); // Удалить в случае если целостность не нарушена.
					}

					result = ics_service::IcsStatusCode::type::Ics_Success;
					return true;
				}
				else
				{
					//result = ics_service::IcsStatusCode::type::Ics_???;
					m_log.print(std::string(__FUNCTION__) + " error - couldn't recalculate hash for: " + strings::ws_s(_filepath));
				}

			} else
			{
				result = ics_service::IcsStatusCode::type::Ics_FileNotFound;
				m_log.print(std::string(__FUNCTION__) + " error - object is not in a control area: " + strings::ws_s(_filepath));
			}

			return false;
		}

		// Тут нужно быть осторожным, в следующей ситуации - 
		// Во время перерасчёта некоторый объекты могут быть удалены из области контроля целостности, 
		// а затем могут быть добавлены снова после перерасчета.
		bool FilesControl::recalculate()
		{
			FilesMap filesList;
			m_storage.toMap(filesList);

			for (auto file : filesList)
			{
				bool changed;
				ics_service::IcsStatusCode::type result;
				bool success = recalculate(file.first, changed, result);
			}

			return true;
		}


		// Добавляет ключ в список изменённых, только если такой ключ имеется в списке контролируемых.
		void FilesControl::markAsChanged(const FileDescription& _fileInfo)
		{
			if (m_storage.setFileStatus(_fileInfo.szFilePath, STATE_CHANGED))
			{
				///// Отправка события безопасности
				szi::OperationResult result;
				result.errorCode = szi::SziStatusCodes::NoError;
				result.status = "NoError";
				getEventEmiter()->IntCntrlFileBrk("system", result, strings::ws_s(_fileInfo.szFilePath, CP_UTF8));
			}	
		}

		void FilesControl::markAsLocked(const std::wstring& _file)
		{
			m_storage.setFileStatus(_file, STATE_LOCKED);
		}

		void FilesControl::markAsNormal(const std::wstring& _file)
		{
			m_storage.setFileStatus(_file, STATE_NORMAL);
		}

		void FilesControl::markAsMissed(const std::wstring& _file)
		{
			m_storage.setFileStatus(_file, STATE_MISSED);

		///// Отправка события безопасности
			szi::OperationResult result;
			result.errorCode = szi::SziStatusCodes::NoError;
			result.status = "NoError";
			getEventEmiter()->IntCntrlFileBrk("system", result, strings::ws_s(_file, CP_UTF8));
		}

		void FilesControl::getList(FilesMap& _outFiles) const
		{
			m_storage.toMap(_outFiles);
		}

		void FilesControl::getListChanged(FilesMap& _outFiles) const
		{
			FilesMap files;
			m_storage.toMap(files);
			for (auto f : files)
			{
				if (f.second.status == STATE_CHANGED)
				{
					_outFiles[f.first] = f.second;
				}
			}
		}

		void FilesControl::getMissed(std::set<std::wstring>& _outMissedFiles) const
		{
			FilesMap files;
			m_storage.toMap(files);
			for (auto f : files)
			{
				if (f.second.status == STATE_MISSED)
				{
					_outMissedFiles.insert(f.first);
				}
			}
		}

		void FilesControl::getLocked(std::set<std::wstring>& _outFiles) const
		{
			FilesMap files;
			m_storage.toMap(files);
			for (auto f : files)
			{
				if (f.second.status == STATE_LOCKED)
				{
					_outFiles.insert(f.first);
				}
			}
		}

		bool FilesControl::hasChangedObjects() const
		{
			FilesMap fmap;
			getListChanged(fmap);
			return !fmap.empty();
		}

		bool FilesControl::isMissed(std::wstring _file)
		{
			std::set<std::wstring> missedFiles;
			getMissed(missedFiles);
			return missedFiles.count(_file) != 0;
		}

		bool FilesControl::hasMissedFiles() const
		{
			std::set<std::wstring> missedFiles;
			getMissed(missedFiles);
			return missedFiles.size() != 0;
		}

		bool FilesControl::isLocked(std::wstring _file)
		{
			std::set<std::wstring> files;
			getLocked(files);
			return files.count(_file) != 0;
		}

		// Возвращает список доступных алгоритмов хэширования
		std::vector<ics_service::HashAlg::type> FilesControl::getHashesList()
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

		std::shared_ptr<events_client::EventEmiter> FilesControl::getEventEmiter()
		{
			return events_client::EventEmiterHolder::get("127.0.0.1", 
														 szi::Settings::get().getSziEventsLocalPort(m_log.getLogFilePath()),
														 m_log.getLogFilePath());
		}
	}
}
