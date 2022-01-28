

#pragma once

#include "async_handling.h"
#include "../core/ics_core.h"
#include <boost/noncopyable.hpp>
#include "../../log.h"
#include <atomic>
#include <memory>

namespace ics
{
	class AsyncRequestsDispatcher : private boost::noncopyable
	{
	public:

		AsyncRequestsDispatcher(logfile& _log, ics::Core& _icsCore, ics::AsyncRequestsKeeper& _requestsKeeper):
			m_log(_log),
			m_icsCore(_icsCore),
			m_requestsKeeper(_requestsKeeper)
		{
			m_activeHandlers = 0;
		}

		ics_service::AsyncOperationResult recalculateKey(std::wstring _keypath);
		ics_service::AsyncOperationResult recalculateAllKeys();

		ics_service::AsyncOperationResult recalculateFile(std::wstring _filepath);
		ics_service::AsyncOperationResult recalculateAllFiles();

		ics_service::AsyncOperationResult recalculateDir(std::wstring _dirpath);
		ics_service::AsyncOperationResult recalculateAllDirs();

		unsigned long getCountOfActiveHandlers();

	private:
		static void handler_recalculateKey(ics_service::AsyncOperationId _requestId, AsyncRequestsDispatcher& _dispatcher, std::wstring _keypath);
		static void handler_recalculateAllKeys(ics_service::AsyncOperationId _requestId, AsyncRequestsDispatcher& _dispatcher);

		static void handler_recalculateFile(ics_service::AsyncOperationId _requestId, AsyncRequestsDispatcher& _dispatcher, std::wstring _filepath);
		static void handler_recalculateAllFiles(ics_service::AsyncOperationId _requestId, AsyncRequestsDispatcher& _dispatcher);

		static void handler_recalculateDir(ics_service::AsyncOperationId _requestId, AsyncRequestsDispatcher& _dispatcher, std::wstring _dirpath);
		static void handler_recalculateAllDirs(ics_service::AsyncOperationId _requestId, AsyncRequestsDispatcher& _dispatcher);

		ics_service::AsyncOperationResult makeResult();

		logfile& log() const { return m_log; }
		ics::Core& core() const { return m_icsCore; };
		ics::AsyncRequestsKeeper& requests() const { return m_requestsKeeper; }

		// Register start and finish of handling process.
		// It's need to know how many request are handling in a current moment of time.
		void handlerStarted();
		void handlerCompleted();

	private:
		logfile& m_log;
		ics::Core& m_icsCore;
		std::atomic_ulong m_activeHandlers;
		ics::AsyncRequestsKeeper& m_requestsKeeper;

	};
}
