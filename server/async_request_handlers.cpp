

#include "async_request_handlers.h"
#include <thread>

namespace ics
{
	ics_service::AsyncOperationResult AsyncRequestsDispatcher::recalculateKey(std::wstring _keypath)
	{
		// Register the fact of handling request.
		handlerStarted();

		ics_service::AsyncOperationResult result = makeResult();
		std::thread(AsyncRequestsDispatcher::handler_recalculateKey, result.operation_number, std::ref(*this), _keypath).detach();
		return result;

		//return asyncCall(*this, AsyncRequestsDispatcher::handler_recalculateKey, _keypath, result.operation_number, std::ref(*this));
	}

	ics_service::AsyncOperationResult AsyncRequestsDispatcher::recalculateAllKeys()
	{
		handlerStarted();

		ics_service::AsyncOperationResult result = makeResult();
		std::thread(AsyncRequestsDispatcher::handler_recalculateAllKeys, result.operation_number, std::ref(*this)).detach();
		return result;
	}

	ics_service::AsyncOperationResult AsyncRequestsDispatcher::recalculateFile(std::wstring _filepath)
	{
		handlerStarted();

		ics_service::AsyncOperationResult result = makeResult();
		std::thread(AsyncRequestsDispatcher::handler_recalculateFile, result.operation_number, std::ref(*this), _filepath).detach();
		return result;
	}

	ics_service::AsyncOperationResult AsyncRequestsDispatcher::recalculateAllFiles()
	{
		handlerStarted();

		ics_service::AsyncOperationResult result = makeResult();
		std::thread(AsyncRequestsDispatcher::handler_recalculateAllFiles, result.operation_number, std::ref(*this)).detach();
		return result;
	}

	//////////////////////////////////////////////////////////////////////////
	// dirs

	ics_service::AsyncOperationResult AsyncRequestsDispatcher::recalculateDir(std::wstring _dirpath)
	{
		handlerStarted();

		ics_service::AsyncOperationResult result = makeResult();
		std::thread(AsyncRequestsDispatcher::handler_recalculateDir, result.operation_number, std::ref(*this), _dirpath).detach();
		return result;
	}

	ics_service::AsyncOperationResult AsyncRequestsDispatcher::recalculateAllDirs()
	{
		handlerStarted();

		ics_service::AsyncOperationResult result = makeResult();
		std::thread(AsyncRequestsDispatcher::handler_recalculateAllDirs, result.operation_number, std::ref(*this)).detach();
		return result;
	}

	void AsyncRequestsDispatcher::handler_recalculateKey(ics_service::AsyncOperationId _requestId, AsyncRequestsDispatcher& _dispatcher, std::wstring _keypath)
	{
		bool isChanged = false;
		ics_service::IcsStatusCode::type result;
		if (_dispatcher.m_icsCore.regControl().recalculate(_keypath, isChanged, result))
		{
			_dispatcher.m_icsCore.updateLastFullVerificationTime();
		}
		else
		{
			_dispatcher.log().print(std::string(__FUNCTION__) + " failed for: " + strings::ws_s(_keypath));
		}
		_dispatcher.requests().setRequestState(_requestId, result);

		_dispatcher.handlerCompleted();
	}

	void AsyncRequestsDispatcher::handler_recalculateAllKeys(ics_service::AsyncOperationId _requestId, AsyncRequestsDispatcher& _dispatcher)
	{
		if (_dispatcher.m_icsCore.regControl().recalculate())
		{
			_dispatcher.m_icsCore.updateLastFullVerificationTime();
			_dispatcher.requests().setRequestState(_requestId, ics_service::IcsStatusCode::Ics_Success);
		}
		else
		{
			_dispatcher.log().print(std::string(__FUNCTION__) + " failed.");
			_dispatcher.requests().setRequestState(_requestId, ics_service::IcsStatusCode::Ics_Error);
		}

		_dispatcher.handlerCompleted();
	}

	void AsyncRequestsDispatcher::handler_recalculateFile(ics_service::AsyncOperationId _requestId, AsyncRequestsDispatcher& _dispatcher, std::wstring _filepath)
	{
		bool isChanged = false;
		ics_service::IcsStatusCode::type result;
		if (_dispatcher.m_icsCore.filesControl().recalculate(_filepath, isChanged, result))
		{
			_dispatcher.m_icsCore.updateLastFullVerificationTime();
		}
		else
		{
			_dispatcher.log().print(std::string(__FUNCTION__) + " failed for: " + strings::ws_s(_filepath));
		}
		_dispatcher.requests().setRequestState(_requestId, result);

		_dispatcher.handlerCompleted();
	}

	void AsyncRequestsDispatcher::handler_recalculateAllFiles(ics_service::AsyncOperationId _requestId, AsyncRequestsDispatcher& _dispatcher)
	{
		bool isChanged = false;
		if (_dispatcher.m_icsCore.filesControl().recalculate())
		{
			_dispatcher.m_icsCore.updateLastFullVerificationTime();
			_dispatcher.requests().setRequestState(_requestId, ics_service::IcsStatusCode::Ics_Success);
		}
		else
		{
			_dispatcher.log().print(std::string(__FUNCTION__) + " failed.");
			_dispatcher.requests().setRequestState(_requestId, ics_service::IcsStatusCode::Ics_Error);
		}

		_dispatcher.handlerCompleted();
	}

	void AsyncRequestsDispatcher::handler_recalculateDir(ics_service::AsyncOperationId _requestId, AsyncRequestsDispatcher& _dispatcher, std::wstring _dirpath)
	{
		bool isChanged = false;
		ics_service::IcsStatusCode::type result;
		if (_dispatcher.m_icsCore.dirControl().recalculate(_dirpath, isChanged, result))
		{
			_dispatcher.m_icsCore.updateLastFullVerificationTime();
		}
		else
		{
			_dispatcher.log().print(std::string(__FUNCTION__) + " failed for: " + strings::ws_s(_dirpath));
		}
		_dispatcher.requests().setRequestState(_requestId, result);

		_dispatcher.handlerCompleted();
	}

	void AsyncRequestsDispatcher::handler_recalculateAllDirs(ics_service::AsyncOperationId _requestId, AsyncRequestsDispatcher& _dispatcher)
	{
		bool isChanged = false;
		if (_dispatcher.m_icsCore.dirControl().recalculate())
		{
			_dispatcher.m_icsCore.updateLastFullVerificationTime();
			_dispatcher.requests().setRequestState(_requestId, ics_service::IcsStatusCode::Ics_Success);
		}
		else
		{
			_dispatcher.log().print(std::string(__FUNCTION__) + " failed.");
			_dispatcher.requests().setRequestState(_requestId, ics_service::IcsStatusCode::Ics_Error);
		}

		_dispatcher.handlerCompleted();
	}

	ics_service::AsyncOperationResult AsyncRequestsDispatcher::makeResult()
	{
		ics_service::AsyncOperationResult result;
		result.operation_number = m_requestsKeeper.createNewRequest(ics_service::IcsStatusCode::Ics_NotFinishedYet);
		result.result = ics_service::IcsStatusCode::Ics_Success;
		return result;
	}

	void AsyncRequestsDispatcher::handlerStarted()
	{
		m_activeHandlers.operator++();
	}

	void AsyncRequestsDispatcher::handlerCompleted()
	{
		m_activeHandlers.operator--();
	}

	unsigned long AsyncRequestsDispatcher::getCountOfActiveHandlers()
	{
		return m_activeHandlers.load();
	}
}
