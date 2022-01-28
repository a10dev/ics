

#include "ics_server.h"
#include "../../management/settings/sziLocalSettings.h"
#include "../../AutoTest/SziAutoTest.h"
#include "../../SelfControl/SziSelfControl.h"


namespace ics
{	
	////////////////////////////////////////////////////////////////////////////
	// C  O  M  P  O  N  E  N  T  S
	
	IcsStatusCode::type IntegrityControlServer::recalculateComponent(const session_id& id)
	{
		SelfControl::SziSelfControl::RunSelfControlComponents();
		return IcsStatusCode::Ics_Success;
	}

	void IntegrityControlServer::getComponentStatus(ResponseStatus& _return, const session_id& id)
	{
		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			m_log.print(std::string(__FUNCTION__) + ": access denied");
			_return.state = ObjectState::Unknown;
			_return.result = IcsStatusCode::Ics_AccessDenied;
			return;
		}

		SelfControl::SziSelfControl::SziTestResult	lResultTest;
		long long									lLastTime = 0;
		SelfControl::SziSelfControl::Instance().GetComponentsLastResult(lResultTest, lLastTime);

		_return.state = lResultTest.empty() || std::get<0>(lResultTest.front()).GetResult() ?  ObjectState::Normal :   ObjectState::Compromised;
		m_log.print(std::string(__FUNCTION__) + " component status: " + std::to_string(_return.state) + " time: " + std::to_string(lLastTime));
		_return.result = IcsStatusCode::Ics_Success;
	}

	void IntegrityControlServer::getLastComponentVerificationTime(DigitTime& _return)
	{
		SelfControl::SziSelfControl::SziTestResult	lResultTest;
		long long									lLastTime = 0;
		SelfControl::SziSelfControl::Instance().GetComponentsLastResult(lResultTest, lLastTime);

		const auto localtime = std::localtime(&lLastTime);
		
		_return._milliseconds = 0;
		_return._second = localtime->tm_sec;
		_return._minute = localtime->tm_min;
		_return._hour   = localtime->tm_hour;
		_return._day    = localtime->tm_mday;
		_return._month  = localtime->tm_mon + 1;
		_return._year   = localtime->tm_year + 1900;		
	}


	IcsStatusCode::type IntegrityControlServer::setComponentVerification(const session_id& _id, const AutoVerification& _settings)
	{
		m_log.print(std::string(__FUNCTION__));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(_id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			m_log.print(std::string(__FUNCTION__) + ": access denied");
			return IcsStatusCode::Ics_AccessDenied;
		}
			
		const SelfControl::SziSelfControlConfig selfControlConfig(_settings.repeatTimeout, _settings.autostart, _settings.action);
		SelfControl::SziSelfControl::Instance().SetComponentsConfig(selfControlConfig);

		return IcsStatusCode::Ics_Success;
	}

	void IntegrityControlServer::getComponentVerification(ResponseAutoVerification& _return, const session_id& _id)
	{
		m_log.print(std::string(__FUNCTION__));

		if (::server::GeneralUserSessionsStorage::get().verifyAccessRight(_id, ::szi::AccessRight::type::IntegrityControl) != ::server::SessionManager::ACCESS_SUCCESS)
		{
			m_log.print(std::string(__FUNCTION__) + ": access denied");
			_return.result = IcsStatusCode::Ics_AccessDenied;
			return;
		}

		_return.settings.repeatTimeout = szi::Settings::get().icsComponentsGetVerificationTimeout();
		_return.settings.autostart = szi::Settings::get().icsComponentsIsEnabledAutoScan();

		std::string compromiseAction = szi::Settings::get().icsComponentsGetCompromisedValue();

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
