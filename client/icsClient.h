

#pragma once

#include <string>
#include "../../../../thrift/cpp/ics/IcsService.h"
#include "../../../../thrift/cpp/registerARM/securityEventTypes_types.h"
#include <transport/TSocket.h>
#include <protocol/TBinaryProtocol.h>

namespace integrity 
{	
	using namespace ::apache::thrift;
	using namespace ::apache::thrift::protocol;
	using namespace ::apache::thrift::transport;

	class Client
	{
	public:

		Client(int _port, std::string _host = "127.0.0.1");

		bool isIntegrityCompromised() const;

		bool isBootIntegrityCompromised() const;

		bool getSettings(ics_service::IcsSettings& _settings) const;

	private:
		std::string m_host;
		int m_port;
		boost::shared_ptr<TTransport> m_transport;
		boost::shared_ptr<TProtocol> m_protocol;
		mutable ics_service::IcsServiceClient m_client;
	};
}
