

#include "ics_server.h"
#include "../../management/settings/sziLocalSettings.h"
#include "../../AutoTest/SziAutoTest.h"
#include "../../SelfControl/SziSelfControl.h"


namespace ics
{	
	////////////////////////////////////////////////////////////////////////////
	// B  O  O  T    S  E  C  T  O  R

	IcsStatusCode::type IntegrityControlServer::recalculateBoot(const session_id& id)
	{
		m_log.print(std::string(__FUNCTION__));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			return IcsStatusCode::Ics_AccessDenied;
		}
		else
		{
			if (m_icsCore.bootControl().recalculate())
			{
				return ics_service::IcsStatusCode::Ics_Success;
			} 
			else
			{
				return ics_service::IcsStatusCode::Ics_Error;
			}
		}
	}

	void IntegrityControlServer::getBootInfo(ResponseControlledResource& _return, const session_id& id)
	{
		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			_return.result = IcsStatusCode::Ics_AccessDenied;
			return;
		}

		auto bootDescription = m_icsCore.bootControl().getDescription();

		ics_service::ControlledResource res;
		res.id = "boot_sector";
		res.hashAlgorithm = static_cast<ics_service::HashAlg::type>(bootDescription.algorithm);
		res.included_objects = false;
		res.creation_time = timefn::getTime(bootDescription.time);
		res.broke_time = timefn::getCurrentTime();
		res.state = ObjectState::Normal;

		if (bootDescription.status == STATE_CHANGED)
		{
			res.state = ObjectState::Compromised;
		}
		
		_return.resource = res;
		_return.result = IcsStatusCode::Ics_Success;		
	}


	void IntegrityControlServer::isBootIntegrityCompromised(IcsBool& _return)
	{
		_return.isTrue = m_icsCore.bootControl().isCompromised();
		_return.requestsState = IcsStatusCode::Ics_Success;
	}

	void IntegrityControlServer::getLastBootVerificationTime(DigitTime& _return)
	{
		const auto time = m_icsCore.bootControl().getDescription().time;
		_return._day = time.wDay;
		_return._hour = time.wHour;
		_return._month = time.wMonth;
		_return._year = time.wYear;
		_return._minute = time.wMinute;
		_return._second = time.wSecond;
		_return._milliseconds = time.wMilliseconds;
	}

	//
	// Установка параметров проверки целостности проверки загрузочного сектора.
	//
	IcsStatusCode::type IntegrityControlServer::setBootVerification(const session_id& _id, const AutoVerification& _settings)
	{
		m_log.print(std::string(__FUNCTION__));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(_id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			m_log.print(std::string(__FUNCTION__) + ": access denied");
			return IcsStatusCode::Ics_AccessDenied;
		}

		if (m_icsCore.applyBootSettings(_settings))
		{
			return IcsStatusCode::Ics_Success;
		}
		else
		{
			m_log.print(std::string(__FUNCTION__) + " error - couldn't update boot settings of the program.");
			return IcsStatusCode::Ics_Error;
		}
	}

	//
	// Получение параметров о целостности загрузочного сектора.
	//
	void IntegrityControlServer::getBootVerification(ResponseAutoVerification& _return, const session_id& _id)
	{
		m_log.print(std::string(__FUNCTION__));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(_id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			m_log.print(std::string(__FUNCTION__) + ": access denied");
			_return.result = IcsStatusCode::Ics_AccessDenied;
			return;
		}

		_return.settings.repeatTimeout = szi::Settings::get().icsBootGetVerificationTimeout();
		_return.settings.autostart = szi::Settings::get().icsBootIsEnabledAutoScan();

		std::string compromiseAction = szi::Settings::get().icsBootGetCompromisedValue();

		if (strings::equalStrings(compromiseAction, "lock"))
		{
			_return.settings.action = ics::AutoAction::LockSystem;
		}
		else if (strings::equalStrings(compromiseAction, "ignore"))
		{
			_return.settings.action = ics::AutoAction::Ignore;
		}

		_return.result = IcsStatusCode::Ics_Success;
	}
}
