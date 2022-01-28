

#include "ics_server.h"
#include "../../management/settings/sziLocalSettings.h"
#include "../../AutoTest/SziAutoTest.h"
#include "../../SelfControl/SziSelfControl.h"


namespace ics
{
	szi::OperationResult toOperationResult(IcsStatusCode::type _icsType)
	{
		szi::OperationResult result;
		switch (_icsType)
		{
		case IcsStatusCode::Ics_Success:
			result.errorCode = szi::SziStatusCodes::NoError;
			result.status = "Ics_Success";
			break;
		case IcsStatusCode::Ics_Error:
			result.errorCode = szi::SziStatusCodes::ServiceError;
			result.status = "Ics_Error";
			break;
		case IcsStatusCode::Ics_InvalidParameter:
			result.errorCode = szi::SziStatusCodes::ServiceError;
			result.status = "Ics_InvalidParameter";
			break;
		case IcsStatusCode::Ics_NotFound:
			result.errorCode = szi::SziStatusCodes::ServiceError;
			result.status = "Ics_NotFound";
			break;
		case IcsStatusCode::Ics_Unknown:
			result.errorCode = szi::SziStatusCodes::ServiceError;
			result.status = "Ics_Unknown";
			break;
		case IcsStatusCode::Ics_RequestPostedInQueue:
			result.errorCode = szi::SziStatusCodes::ServiceError;
			result.status = "Ics_RequestPostedInQueue";
			break;
		case IcsStatusCode::Ics_Ignored:
			result.errorCode = szi::SziStatusCodes::ServiceError;
			result.status = "Ics_Ignored";
			break;
		case IcsStatusCode::Ics_AccessDenied:
			result.errorCode = szi::SziStatusCodes::ServiceError;
			result.status = "Ics_AccessDenied";
			break;
		case IcsStatusCode::Ics_Finished:
			result.errorCode = szi::SziStatusCodes::ServiceError;
			result.status = "Ics_Finished";
			break;
		case IcsStatusCode::Ics_NotFinishedYet:
			result.errorCode = szi::SziStatusCodes::ServiceError;
			result.status = "Ics_NotFinishedYet";
			break;
		case IcsStatusCode::Ics_WrongId:
			result.errorCode = szi::SziStatusCodes::ServiceError;
			result.status = "Ics_WrongId";
			break;
		case IcsStatusCode::Ics_UnknownAlgorithm:
			result.errorCode = szi::SziStatusCodes::UnknownAlgorithm;
			result.status = "Ics_UnknownAlgorithm";
			break;
		case IcsStatusCode::Ics_FsObjectAccessError:
			result.errorCode = szi::SziStatusCodes::FsObjectAccessError;
			result.status = "Ics_FsObjectAccessError";
			break;
		default:
			result.errorCode = szi::SziStatusCodes::NoError;
			result.status = "Ics_Success";
			break;
		}
		return result;
	}

	IntegrityControlServer::~IntegrityControlServer()
	{
		m_log.print(std::string(__FUNCTION__) + ": wait for terminating async request handlers...");

		waitForAsyncRequestsHandled();

		m_log.print(std::string(__FUNCTION__) + ": all async requests are finished. The service is ready to stop.");
	}

	std::shared_ptr<events_client::EventEmiter> IntegrityControlServer::getEventEmiter()
	{
		return events_client::EventEmiterHolder::get("127.0.0.1", 
													 szi::Settings::get().getSziEventsLocalPort(m_log.getLogFilePath()),
													 m_log.getLogFilePath());
	}

	void IntegrityControlServer::waitForAsyncRequestsHandled()
	{
		// Active wait when async handlers will be finished.
		for (; m_handlers.getCountOfActiveHandlers() != 0 ;);
	}

	void IntegrityControlServer::isIntegrityCompromised(IcsBool& _return)
	{
		_return.isTrue = m_icsCore.regControl().isCompromised() || 
						 m_icsCore.filesControl().isCompromised() ||
						 m_icsCore.dirControl().isCompromised(); 

		_return.requestsState = IcsStatusCode::Ics_Success;

		//
		// I turned off that print, because of a lot of trash in a log.
		//
		// m_log.print(std::string(__FUNCTION__) + ": " + (_return.isTrue ? "True" : "False"));
	}

	void IntegrityControlServer::getLastVerificationTime(DigitTime& _return)
	{
		const auto t = m_icsCore.getLastFullVerificationTime();
		_return._day = t.wDay;
		_return._hour = t.wHour;
		_return._month = t.wMonth;
		_return._year = t.wYear;
		_return._minute = t.wMinute;
		_return._second = t.wSecond;
		_return._milliseconds = t.wMilliseconds;

		//
		// I turned off that print, because of a lot of trash in a log.
		//
		//m_log.print(std::string(__FUNCTION__) + " last full verification time: " + timefn::getTime(t));
	}

	IcsStatusCode::type IntegrityControlServer::getStateOfAsyncOperation(const AsyncOperationId asyncOperationNumber)
	{
		wsprintfA(m_log.acquireBuffer(), "%s : request id is %i64", __FUNCTION__, asyncOperationNumber);
		m_log.leaveBuffer();

		IcsStatusCode::type state;
		if (m_requestsHistory.getRequestState(asyncOperationNumber, state))
		{
			return state;
		}

		// Возвращается в случае, если передан идентификатор несуществующего запроса.
		return IcsStatusCode::Ics_WrongId;
	}

	IcsStatusCode::type IntegrityControlServer::setSettings(const session_id& id, const IcsSettings& settings)
	{
		m_log.print(std::string(__FUNCTION__));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			return IcsStatusCode::Ics_AccessDenied;
		}

		if (m_icsCore.applySettings(settings))
		{
			return IcsStatusCode::Ics_Success;
		}
		else
		{
			m_log.print(std::string(__FUNCTION__) + " error - couldn't update settings of the program.");
			return IcsStatusCode::Ics_Error;
		}
	}

	void IntegrityControlServer::getSettings(ResponseSettings& _return, const session_id& id)
	{
		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			_return.result = IcsStatusCode::Ics_AccessDenied;
		}
		else
		{
			_return.settings = m_icsCore.getConfig().get();
			_return.result = IcsStatusCode::Ics_Success;
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// R  E  G  I  S  T  R  Y

	IcsStatusCode::type IntegrityControlServer::addKey(const session_id& _user_session,
		const std::string& _keypath,
		const bool _include_subkeys,
		const HashAlg::type _algorithm)
	{
		IcsStatusCode::type result = IcsStatusCode::Ics_Error;
		std::wstring key = strings::s_ws(_keypath, CP_UTF8);
		m_log.print(std::string(__FUNCTION__) + " : " + strings::ws_s(key));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(_user_session, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			result = IcsStatusCode::Ics_AccessDenied;
		}
		else
		{
			if (!m_icsCore.regControl().addKey(key, _include_subkeys, _algorithm, result))
			//{
			//	result = IcsStatusCode::Ics_Success;
			//}
			//else
			//{
				m_log.print(std::string(__FUNCTION__) + " error - couldn't add a key. error code: " + std::to_string(static_cast<int>(result)));
				// result = IcsStatusCode::Ics_Error; уже присвоено изначально
				// и уточнено внутри addKey(...)
			//}
		}

	//////// Отправка события безопасности ////////
		::server::UserSessionInfo info;
		::server::GeneralUserSessionsStorage::get().getSessionInfo(_user_session, info);
		getEventEmiter()->AddIntCntrlReg(info.authResult.username, toOperationResult(result), _keypath);
		return result;
	}

	IcsStatusCode::type IntegrityControlServer::removeKey(const session_id& id, const std::string& keypath)
	{
		IcsStatusCode::type result = IcsStatusCode::Ics_Error;
		std::wstring key = strings::s_ws(keypath, CP_UTF8);
		m_log.print(std::string(__FUNCTION__) + " : " + strings::ws_s(key));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			result = IcsStatusCode::Ics_AccessDenied;
		}
		else
		{
			if (!m_icsCore.regControl().removeKey(key))
			{
				m_log.print(std::string(__FUNCTION__) + " error - couldn't remove a key.");
				result = IcsStatusCode::Ics_KeyNotFound;
			}
			else {
				result = IcsStatusCode::Ics_Success;
			}
		}

		::server::UserSessionInfo info;
		::server::GeneralUserSessionsStorage::get().getSessionInfo(id, info);
		getEventEmiter()->DelIntCntrlReg(info.authResult.username, toOperationResult(result), keypath);
		return result;
	}

	void IntegrityControlServer::getKeyInfo(ResponseControlledResource& _return, const session_id& id, const std::string& _keypath)
	{
		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			_return.result = IcsStatusCode::Ics_AccessDenied;
			return;
		}

		std::wstring keypath = strings::s_ws(_keypath, CP_UTF8);
		strings::toUpper(keypath);

		ics::registry::KeyMap list, changedKeys;
		m_icsCore.regControl().getListKeys(list);
		m_icsCore.regControl().getListChangedKeys(changedKeys);

		auto pos = list.find(keypath);
		if (pos != list.cend())
		{
			ics_service::ControlledResource res;
			res.id = _keypath;
			res.hashAlgorithm = static_cast<ics_service::HashAlg::type>(pos->second.algorithm);
			res.included_objects = pos->second.subKeysIncluded;
			res.creation_time = timefn::getTime(pos->second.time);
			res.broke_time = timefn::getCurrentTime();
			res.state = ObjectState::Normal;
			res.sizeInfo = pos->second.countObjects;

			if (changedKeys.find(keypath) != changedKeys.cend())
			{
				res.state = ObjectState::Compromised;
			}

			auto missed = m_icsCore.regControl().getMissedKeys();
			if (!missed.empty())
			{
				res.state = ((missed.count(keypath) != 0) ? ObjectState::NotFound : res.state);
			}

			_return.resource = res;
			_return.result = IcsStatusCode::Ics_Success;
		}
		else
		{
			_return.result = IcsStatusCode::Ics_KeyNotFound;
		}
	}

	void IntegrityControlServer::getListKeys(ResponseWithListOfControlledResources& _return) 
	{
		ics::registry::KeyMap list, changedKeys;
		m_icsCore.regControl().getListKeys(list);
		m_icsCore.regControl().getListChangedKeys(changedKeys);

		for (auto i : list)
		{
			ics_service::ControlledResource res;
			res.id = strings::ws_s(i.first, CP_UTF8);
			res.hashAlgorithm = static_cast<ics_service::HashAlg::type>(i.second.algorithm);
			res.included_objects = i.second.subKeysIncluded;
			res.creation_time = timefn::getTime(i.second.time);
			res.broke_time = timefn::getCurrentTime();
			res.state = ObjectState::Normal;
			res.sizeInfo = i.second.countObjects;

			if (changedKeys.find(i.first) != changedKeys.cend())
			{
				res.state = ObjectState::Compromised;
			}

			auto missed = m_icsCore.regControl().getMissedKeys();
			if (!missed.empty())
			{
				res.state = ((missed.count(i.first) != 0) ? ObjectState::NotFound : res.state);
			}

			_return.resourses.push_back(res);
		}

		_return.error_code = IcsStatusCode::Ics_Success;
	}

	IcsStatusCode::type IntegrityControlServer::changeHashAlgorithmForKey(const session_id& id, const std::string& _keypath, const HashAlg::type _algorithm)
	{
		std::wstring key = strings::s_ws(_keypath, CP_UTF8);
		m_log.print(std::string(__FUNCTION__) + " for: " + strings::ws_s(key));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			return IcsStatusCode::Ics_AccessDenied;
		}
		else
		{
			IcsStatusCode::type result;
			if (!m_icsCore.regControl().changeHashAlgorithm(key, _algorithm, result))
			//{
			//	return IcsStatusCode::Ics_Success;
			//}
			//else
			//{
				m_log.print(std::string(__FUNCTION__) + " error - can't change hashing algorithm.");
				//return IcsStatusCode::Ics_Error;
			//}
			return result;
		}
	}

	void IntegrityControlServer::recalculateKey(AsyncOperationResult& _return, const session_id& id, const std::string& keypath)
	{
		std::wstring key = strings::s_ws(keypath, CP_UTF8);
		m_log.print(std::string(__FUNCTION__) + " for: " + strings::ws_s(key));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			_return.result = IcsStatusCode::Ics_AccessDenied;
		}
		else
		{
			_return = m_handlers.recalculateKey(key);
		}
	}

	void IntegrityControlServer::recalculateAllKeys(AsyncOperationResult& _return, const session_id& id)
	{
		m_log.print(std::string(__FUNCTION__));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			_return.result = IcsStatusCode::Ics_AccessDenied;
		}
		else
		{
			_return = m_handlers.recalculateAllKeys();
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// F   I   L   E   S

	IcsStatusCode::type IntegrityControlServer::addFile(const session_id& user_session,
		const std::string& _filepath,
		const HashAlg::type _algorithm)
	{
		IcsStatusCode::type result = IcsStatusCode::Ics_Error;
		std::wstring file = strings::s_ws(_filepath, CP_UTF8);
		m_log.print(std::string(__FUNCTION__) + " : " + strings::ws_s(file));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(user_session, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			result = IcsStatusCode::Ics_AccessDenied;
		}
		else
		{
			windir::FsTypes fileType;
			if (!windir::getFsObjectType(file, fileType))
			{
				return IcsStatusCode::Ics_FsObjectAccessError;
			}

			if (!m_icsCore.filesControl().add(file, _algorithm, result))
			//{
				m_log.print(std::string(__FUNCTION__) + " error - couldn't add the file.");
			//	result = IcsStatusCode::Ics_Error;
			//}
			//else {
			//	result = IcsStatusCode::Ics_Success;
			//}
		}

		::server::UserSessionInfo info;
		::server::GeneralUserSessionsStorage::get().getSessionInfo(user_session, info);
		getEventEmiter()->AddIntCntrlFile(info.authResult.username, toOperationResult(result), _filepath);
		return result;
	}

	IcsStatusCode::type IntegrityControlServer::removeFile(const session_id& id, const std::string& _filepath)
	{
		IcsStatusCode::type result = IcsStatusCode::Ics_Error;
		std::wstring file = strings::s_ws(_filepath, CP_UTF8);
		m_log.print(std::string(__FUNCTION__) + " : " + strings::ws_s(file));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			result = IcsStatusCode::Ics_AccessDenied;
		}
		else
		{
			if (!m_icsCore.filesControl().remove(file))
			{
				m_log.print(std::string(__FUNCTION__) + " error - couldn't remove the file.");
				result = IcsStatusCode::Ics_FileNotFound;
			}
			else {
				result = IcsStatusCode::Ics_Success;
			}
		}

		::server::UserSessionInfo info;
		::server::GeneralUserSessionsStorage::get().getSessionInfo(id, info);
		getEventEmiter()->DelIntCntrlFile(info.authResult.username, toOperationResult(result), _filepath);
		return result;
	}
	
	void IntegrityControlServer::getFileInfo(ResponseControlledResource& _return, const session_id& id, const std::string& _filepath)
	{
		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			_return.result = IcsStatusCode::Ics_AccessDenied;
			return;
		}

		std::wstring filepath = strings::s_ws(_filepath, CP_UTF8);
		strings::toUpper(filepath);

		ics::files::FilesMap list, changedFiles;
		m_icsCore.filesControl().getList(list);
		m_icsCore.filesControl().getListChanged(changedFiles);

		auto pos = list.find(filepath);
		if (pos != list.cend())
		{
			ics_service::ControlledResource res;
			res.id = _filepath;
			res.sizeInfo = pos->second.fileSize;
			res.hashAlgorithm = static_cast<ics_service::HashAlg::type>(pos->second.algorithm);
			res.included_objects = false;
			res.creation_time = timefn::getTime(pos->second.time);
			res.broke_time = timefn::getCurrentTime();
			res.state = ObjectState::Normal;

			if (changedFiles.find(filepath) != changedFiles.cend())
			{
				res.state = ObjectState::Compromised;
			}
			//	Уточнить статус файла в случае неспецифических изменений.
			// std::set<std::wstring> missedFiles;
			// m_icsCore.filesControl().getMissed(missedFiles)

			res.state = m_icsCore.filesControl().isMissed(filepath) ? ObjectState::NotFound : res.state;
			res.state = m_icsCore.filesControl().isLocked(filepath) ? ObjectState::LockedAccess : res.state;

			_return.resource = res;
			_return.result = IcsStatusCode::Ics_Success;
		}
		else
		{
			_return.result = IcsStatusCode::Ics_FileNotFound;
		}
	}

	void IntegrityControlServer::getListFiles(ResponseWithListOfControlledResources& _return)
	{
		ics::files::FilesMap list, changedFiles;
		m_icsCore.filesControl().getList(list);
		m_icsCore.filesControl().getListChanged(changedFiles);

		std::set<std::wstring> missedFiles, lockedFiles;
		m_icsCore.filesControl().getMissed(missedFiles);
		m_icsCore.filesControl().getLocked(lockedFiles);

		for (auto i : list)
		{
			ics_service::ControlledResource res;
			res.id = strings::ws_s(i.first, CP_UTF8);
			res.hashAlgorithm = static_cast<ics_service::HashAlg::type>(i.second.algorithm);
			res.included_objects = false;
			res.sizeInfo = i.second.fileSize;
			res.creation_time = timefn::getTime(i.second.time);
			res.broke_time = timefn::getCurrentTime();

			if (changedFiles.find(i.first) != changedFiles.cend())
			{
				res.state = ObjectState::Compromised;
			}
			else
			{
				res.state = ObjectState::Normal;
			}

			// Изменить статус, в случае если было исчезновение объекта или закрытие доступа к нему.
			res.state = (missedFiles.count(i.first) != 0) ? ObjectState::NotFound : res.state;
			res.state = (lockedFiles.count(i.first) != 0) ? ObjectState::LockedAccess : res.state;
			_return.resourses.push_back(res);
		}

		_return.error_code = IcsStatusCode::Ics_Success;
	}

	IcsStatusCode::type IntegrityControlServer::changeHashAlgorithmForFile(const session_id& id, const std::string& _filepath, const HashAlg::type _algorithm)
	{
		std::wstring file = strings::s_ws(_filepath, CP_UTF8);
		m_log.print(std::string(__FUNCTION__) + " for: " + strings::ws_s(file));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			return IcsStatusCode::Ics_AccessDenied;
		}

		IcsStatusCode::type result;
		if (!m_icsCore.filesControl().changeHashAlgorithm(file, _algorithm, result))
		//{
		//	return IcsStatusCode::Ics_Success;
		//}
		//else
		//{
			m_log.print(std::string(__FUNCTION__) + " error - can't change hashing algorithm.");
		//	return IcsStatusCode::Ics_Error;
		//}
		return result;
	}

	void IntegrityControlServer::recalculateFile(AsyncOperationResult& _return, const session_id& id, const std::string& _filepath)
	{
		std::wstring filepath = strings::s_ws(_filepath, CP_UTF8);
		m_log.print(std::string(__FUNCTION__) + " for: " + strings::ws_s(filepath));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			_return.result = IcsStatusCode::Ics_AccessDenied;
		}
		else
		{
			_return = m_handlers.recalculateFile(filepath);
		}
	}

	void IntegrityControlServer::recalculateAllFiles(AsyncOperationResult& _return, const session_id& id)
	{
		m_log.print(std::string(__FUNCTION__));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			_return.result = IcsStatusCode::Ics_AccessDenied;
		}
		else
		{
			_return = m_handlers.recalculateAllFiles();
		}
	}

	////////////////////////////////////////////////////////////////////////////
	// D   I   R   E   C   T   O   R   I   E   S

	IcsStatusCode::type IntegrityControlServer::addDirectory(const session_id& user_session,
		const std::string& _dirpath,
		const bool _includeSubdirs,
		const HashAlg::type _algorithm)
	{
		IcsStatusCode::type result = IcsStatusCode::Ics_Error;
		std::wstring dir = strings::s_ws(_dirpath, CP_UTF8);
		m_log.print(std::string(__FUNCTION__) + " : " + strings::ws_s(dir));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(user_session, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			result = IcsStatusCode::Ics_AccessDenied;
		}
		else
		{
			if (!m_icsCore.dirControl().add(dir, _includeSubdirs, _algorithm, result))
			//{
				m_log.print(std::string(__FUNCTION__) + " error - couldn't add dir.");
				//result = IcsStatusCode::Ics_Error;
			//}
			//else
			//	result = IcsStatusCode::Ics_Success;
		}

		::server::UserSessionInfo info;
		::server::GeneralUserSessionsStorage::get().getSessionInfo(user_session, info);
		getEventEmiter()->AddIntCntrlFile(info.authResult.username, toOperationResult(result), _dirpath);
		return result;
	}

	IcsStatusCode::type IntegrityControlServer::removeDirectory(const session_id& user_session, const std::string& _dirpath)
	{
		IcsStatusCode::type result = IcsStatusCode::Ics_Error;
		std::wstring dir = strings::s_ws(_dirpath, CP_UTF8);
		m_log.print(std::string(__FUNCTION__) + " : " + strings::ws_s(dir));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(user_session, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			result = IcsStatusCode::Ics_AccessDenied;
		}
		else
		{
			if (!m_icsCore.dirControl().remove(dir))
			{
				m_log.print(std::string(__FUNCTION__) + " error - couldn't remove dir.");
				result = IcsStatusCode::Ics_Error;
			}
			else result = IcsStatusCode::Ics_Success;
		}

		::server::UserSessionInfo info;
		::server::GeneralUserSessionsStorage::get().getSessionInfo(user_session, info);
		getEventEmiter()->DelIntCntrlFile(info.authResult.username, toOperationResult(result), _dirpath);
		return result;
	}

	void IntegrityControlServer::getDirInfo(ResponseControlledResource& _return, const session_id& id, const std::string& _path)
	{
		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			_return.result = IcsStatusCode::Ics_AccessDenied;
			return;
		}

		std::wstring dirpath = strings::s_ws(_path, CP_UTF8);
		strings::toUpper(dirpath);

		ics::directories::DirsMap list, changedDirs;
		m_icsCore.dirControl().getList(list);
		m_icsCore.dirControl().getListChanged(changedDirs);

		auto pos = list.find(dirpath);
		if (pos != list.cend())
		{
			ics_service::ControlledResource res;
			res.id = _path;
			res.hashAlgorithm = static_cast<ics_service::HashAlg::type>(pos->second.algorithm);
			res.included_objects = false;
			res.creation_time = timefn::getTime(pos->second.time);
			res.broke_time = timefn::getCurrentTime();
			res.state = ObjectState::Normal;
			res.sizeInfo = pos->second.sizeInfo;

			if (changedDirs.find(dirpath) != changedDirs.cend())
			{
				res.state = ObjectState::Compromised;
			}

			//	Уточнить статус директории в случае неспецифических изменений.
			res.state = m_icsCore.dirControl().isMissed(dirpath) ? ObjectState::NotFound : res.state;
			//res.state = m_icsCore.dirControl().isLocked(dirpath) ? ObjectState::LockedAccess : res.state;

			_return.resource = res;
			_return.result = IcsStatusCode::Ics_Success;
		}
		else
		{
			_return.result = IcsStatusCode::Ics_NotFound;
		}
	}

	void IntegrityControlServer::getListDirectories(ResponseWithListOfControlledResources& _return)
	{
		ics::directories::DirsMap list, changedObj;
		m_icsCore.dirControl().getList(list);
		m_icsCore.dirControl().getListChanged(changedObj);

		for (auto i : list)
		{
			ics_service::ControlledResource res;
			res.id = strings::ws_s(i.first, CP_UTF8);
			res.hashAlgorithm = static_cast<ics_service::HashAlg::type>(i.second.algorithm);
			res.included_objects = i.second.includedDirs;
			res.creation_time = timefn::getTime(i.second.time);
			res.broke_time = timefn::getCurrentTime();
			res.state = ObjectState::Normal;
			res.sizeInfo = i.second.sizeInfo;

			if (changedObj.find(i.first) != changedObj.cend())
			{
				res.state = ObjectState::Compromised;
			}
			
			res.state = (m_icsCore.dirControl().getMissed().count(i.first) != 0) ? ObjectState::NotFound : res.state;
			_return.resourses.push_back(res);
		}

		_return.error_code = IcsStatusCode::Ics_Success;
	}

	IcsStatusCode::type IntegrityControlServer::changeHashAlgorithmForDirs(const session_id& id, const std::string& _dirpath, const HashAlg::type _algorithm)
	{
		std::wstring dir = strings::s_ws(_dirpath, CP_UTF8);
		m_log.print(std::string(__FUNCTION__) + " for: " + strings::ws_s(dir));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			return IcsStatusCode::Ics_AccessDenied;
		}
		else
		{
			ics_service::IcsStatusCode::type result;
			if (m_icsCore.dirControl().changeHashAlgorithm(dir, _algorithm, result))
			//{
			//	return IcsStatusCode::Ics_Success;
			//}
			//else
			//{
				m_log.print(std::string(__FUNCTION__) + " error - can't change hashing algorithm.");
			//	return IcsStatusCode::Ics_Error;
			//}
			return result;
		}
	}

	void IntegrityControlServer::recalculateDirectory(AsyncOperationResult& _return, const session_id& id, const std::string& _dirpath)
	{
		std::wstring dirpath = strings::s_ws(_dirpath, CP_UTF8);
		m_log.print(std::string(__FUNCTION__) + " for: " + strings::ws_s(dirpath));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			_return.result = IcsStatusCode::Ics_AccessDenied;
		}
		else
		{
			_return = m_handlers.recalculateDir(dirpath);
		}
	}

	void IntegrityControlServer::recalculateAllDirs(AsyncOperationResult& _return, const session_id& id)
	{
		m_log.print(std::string(__FUNCTION__));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			_return.result = IcsStatusCode::Ics_AccessDenied;
		}
		else
		{
			_return = m_handlers.recalculateAllDirs();
		}
	}


	//////////////////////////////////////////////////////////////////////////////////////

	void IntegrityControlServer::ping(OperationResult& _return)
	{
		_return.errorCode = 0;
		_return.status = strings::ws_s(L"success", CP_UTF8);
	}


	//
	// Запустить полное сканирование целостности всех подсистем. (Файловая система, Реестр, Компоненты СЗИ, Загрузочный сект)
	//
	ics_service::IcsStatusCode::type IntegrityControlServer::verifyAll(const session_id& _id)
	{
		m_log.print(std::string(__FUNCTION__));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(_id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			m_log.print(std::string(__FUNCTION__) + ": access denied");
			return IcsStatusCode::Ics_AccessDenied;
		}

		// Зафиксировать время начала полной проверки целостности.

		m_icsCore.updateLastFullVerificationTime();


		std::thread(Core::recalculateRegFileDir, std::ref(m_icsCore)).detach(); // Файловая система, Реестр
		std::thread(Core::recalculateBoot, std::ref(m_icsCore)).detach();		// Загрузочный сект

		::server::UserSessionInfo lUserInfo;
		::server::GeneralUserSessionsStorage::get().getSessionInfo(_id, lUserInfo);
		AutoTest::SziAutoTestSelfControl componentTest;
		componentTest.Run(lUserInfo); // Компоненты СЗИ.  В будующем возможно передут в icsCore для соответствия GUI.
		
		return IcsStatusCode::Ics_Success;
	}

	// Функция определения списка доступных алгоритмов хэширования
	void IntegrityControlServer::getAllowedHashAlgs(AllowedHashAlgsResponse& _return, const session_id& _id)
	{
		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(_id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			m_log.print(std::string(__FUNCTION__) + ": access denied");
			_return.result = IcsStatusCode::Ics_AccessDenied;
		}

		_return.result = m_icsCore.getAllowedHashAlgs(_return._list);
	}
}
