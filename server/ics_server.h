

#pragma once

#include <memory>
#include "../../stdafx.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include "../../../../thrift/cpp/ics/IcsService.h"
#include "../../helpers.h"
#include "../../management/sessions/SziUserSessions.h"
#include "../core/ics_core.h"
#include "../../log.h"
#include "async_handling.h"
#include "async_request_handlers.h"
#include "../../arm_events_client/event_emiter.h"

namespace ics
{
	using namespace ::apache::thrift;
	using namespace ::apache::thrift::protocol;
	using namespace ::apache::thrift::transport;
	using namespace ::apache::thrift::server;
	using boost::shared_ptr;
	using namespace  ::ics_service;

	class IntegrityControlServer : virtual public ics_service::IcsServiceIf
	{
	public:

		IntegrityControlServer(std::string _logFilePath) :
			m_log(_logFilePath),
			m_handlers(logfile(_logFilePath), m_icsCore, m_requestsHistory)
		{
			m_log.print(std::string(__FUNCTION__));
		}

		// Here server should free all acquired resources and return execution
		// only when all asynchronous requests are handled. 
		~IntegrityControlServer();

		//
		// Внешний интерфейс сервера контроля целостности.
		//
		// regs files dirs
		virtual void isIntegrityCompromised(IcsBool& _return) override;		
		virtual void getLastVerificationTime(DigitTime& _return) override;

		virtual IcsStatusCode::type getStateOfAsyncOperation(const AsyncOperationId asyncOperationNumber) override;
	
		virtual IcsStatusCode::type setSettings(const session_id& id, const IcsSettings& settings) override;
		virtual void getSettings(ResponseSettings& _return, const session_id& id) override;
		// regs
		virtual IcsStatusCode::type addKey(const session_id& user_session, const std::string& keypath, const bool include_subkeys, const HashAlg::type algorithm) override;
		virtual IcsStatusCode::type removeKey(const session_id& id, const std::string& keypath) override;
		virtual void getKeyInfo(ResponseControlledResource& _return, const session_id& id, const std::string& keypath) override;
		virtual void getListKeys(ResponseWithListOfControlledResources& _return) override;
		virtual IcsStatusCode::type changeHashAlgorithmForKey(const session_id& id, const std::string& keypath, const HashAlg::type algorithm) override;
		virtual void recalculateKey(AsyncOperationResult& _return, const session_id& id, const std::string& keypath) override;
		virtual void recalculateAllKeys(AsyncOperationResult& _return, const session_id& id) override;
		// files
		virtual IcsStatusCode::type addFile(const session_id& user_session, const std::string& filepath, const HashAlg::type algorithm) override;
		virtual IcsStatusCode::type removeFile(const session_id& id, const std::string& filepath) override;
		virtual void getFileInfo(ResponseControlledResource& _return, const session_id& id, const std::string& filepath) override;
		virtual void getListFiles(ResponseWithListOfControlledResources& _return) override;
		virtual IcsStatusCode::type changeHashAlgorithmForFile(const session_id& id, const std::string& filepath, const HashAlg::type algorithm) override;
		virtual void recalculateFile(AsyncOperationResult& _return, const session_id& id, const std::string& filepath) override;
		virtual void recalculateAllFiles(AsyncOperationResult& _return, const session_id& id) override;
		// dirs
		virtual IcsStatusCode::type addDirectory(const session_id& user_session, const std::string& dirpath, const bool include_subdirs, const HashAlg::type algorithm) override;
		virtual IcsStatusCode::type removeDirectory(const session_id& user_session, const std::string& dirpath) override;
		virtual void getDirInfo(ResponseControlledResource& _return, const session_id& id, const std::string& path) override;
		virtual void getListDirectories(ResponseWithListOfControlledResources& _return) override;
		virtual IcsStatusCode::type changeHashAlgorithmForDirs(const session_id& id, const std::string& dirpath, const HashAlg::type algorithm) override;
		virtual void recalculateDirectory(AsyncOperationResult& _return, const session_id& id, const std::string& dirpath) override;
		virtual void recalculateAllDirs(AsyncOperationResult& _return, const session_id& id) override;
		// boot
		virtual IcsStatusCode::type recalculateBoot(const session_id& id) override;
		virtual void getBootInfo(ResponseControlledResource& _return, const session_id& id) override;		
		virtual void isBootIntegrityCompromised(IcsBool& _return) override;
		virtual void getLastBootVerificationTime(DigitTime& _return) override;
		virtual IcsStatusCode::type setBootVerification(const session_id& _id, const AutoVerification& _settings) override;
		virtual void getBootVerification(ResponseAutoVerification& _return, const session_id& _id) override;		
		// components
		virtual IcsStatusCode::type recalculateComponent(const session_id& id) override;
		virtual void getComponentStatus(ResponseStatus& _return, const session_id& id) override;
		virtual void getLastComponentVerificationTime(DigitTime& _return) override;
		virtual IcsStatusCode::type setComponentVerification(const session_id& _id, const AutoVerification& _settings) override;
		virtual void getComponentVerification(ResponseAutoVerification& _return, const session_id& _id) override;		
		//
		virtual void ping(OperationResult& _return) override;
		virtual IcsStatusCode::type verifyAll(const session_id& _id) override;


		// Функция определения списка доступных алгоритмов хэширования
		virtual void getAllowedHashAlgs(AllowedHashAlgsResponse& _return, const session_id& _id) override;

		ics::Core* getCore()
		{
			return &m_icsCore;
		}

	protected:

		void waitForAsyncRequestsHandled();

	private:
		// Получить экземпляр EventEmmiter'а
		std::shared_ptr<events_client::EventEmiter> getEventEmiter();


		logfile m_log;
		ics::Core m_icsCore;
		AsyncRequestsKeeper m_requestsHistory;
		AsyncRequestsDispatcher m_handlers;
	};
}
