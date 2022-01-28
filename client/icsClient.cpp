#include "icsClient.h"

#include <iostream>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>

#include <boost/exception/diagnostic_information.hpp>
#include "../../management/sessions/SziUserSessions.h"

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "libthrift.lib")

namespace integrity
{	
	Client::Client(int _port, std::string _host /*= "127.0.0.1" */) : 
		m_host(_host), 
		m_port(_port), 
		m_transport(new TSocket(m_host, m_port)), 
		m_protocol(new TBinaryProtocol(m_transport)),
		m_client(m_protocol) 
	{}

	bool Client::isIntegrityCompromised() const
	{
		try
		{
			m_transport->open();

			ics_service::IcsBool result;
			m_client.isIntegrityCompromised(result);

			m_transport->close();

			if (result.requestsState != ics_service::IcsStatusCode::Ics_Success)
				std::cout << __FUNCTION__ << " return IcsStatusCode: " << result.requestsState << " (" << ics_service::_IcsStatusCode_VALUES_TO_NAMES.at(result.requestsState) << ")" << std::endl;

			return result.isTrue;
		}
		catch (TTransportException& e) // network errors.
		{
			std::cout << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}
		catch (...)
		{
			std::cout << __FUNCTION__ << " unknown exception" << std::endl;
			std::cout << " exception information: " << boost::current_exception_diagnostic_information() << std::endl;
			return false;
		}
	}

	bool Client::isBootIntegrityCompromised() const
	{
		try
		{
			m_transport->open();

			ics_service::IcsBool result;
			m_client.isBootIntegrityCompromised(result);

			m_transport->close();

			if (result.requestsState != ics_service::IcsStatusCode::Ics_Success)
				std::cout << __FUNCTION__ << " return IcsStatusCode: " << result.requestsState << " (" << ics_service::_IcsStatusCode_VALUES_TO_NAMES.at(result.requestsState) << ")" << std::endl;

			return result.isTrue;
		}
		catch (TTransportException& e) // network errors.
		{
			std::cout << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}
		catch (...)
		{
			std::cout << __FUNCTION__ << " unknown exception" << std::endl;
			std::cout << " exception information: " << boost::current_exception_diagnostic_information() << std::endl;
			return false;
		}
	}

	bool Client::getSettings(ics_service::IcsSettings& _settings) const
	{
		try
		{
			m_transport->open();

			ics_service::ResponseSettings result;
			m_client.getSettings(result, ::server::GeneralUserSessionsStorage::get().getInternalSession());

			m_transport->close();

			std::cout << __FUNCTION__ << "settings auto action: " << result.settings.auto_action  << std::endl;

			if (result.result != ics_service::IcsStatusCode::Ics_Success)
				std::cout << __FUNCTION__ << " return IcsStatusCode: " << result.result << " (" << ics_service::_IcsStatusCode_VALUES_TO_NAMES.at(result.result) << ")" << std::endl;

			_settings = result.settings;

			return true;
		}
		catch (TTransportException& e) // network errors.
		{
			std::cout << __FUNCTION__ << " caught TTransportException: " << e.what() << std::endl;
			return false;
		}
		catch (...)
		{
			std::cout << __FUNCTION__ << " unknown exception" << std::endl;
			std::cout << " exception information: " << boost::current_exception_diagnostic_information() << std::endl;
			return false;
		}
	}
}