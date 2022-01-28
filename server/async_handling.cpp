

#include "async_handling.h"


namespace ics
{
	AsyncRequestsKeeper::AsyncRequestsKeeper() :m_counter(0)
	{
		// ...
	}

	bool AsyncRequestsKeeper::isRequestPresent(const ics_service::AsyncOperationId _requestId) const
	{
		return m_operationStates.Present(_requestId);
	}

	ics_service::AsyncOperationId AsyncRequestsKeeper::createNewRequest(ics_service::IcsStatusCode::type requestState /*= ics_service::IcsStatusCode::Ics_NotFinishedYet*/)
	{
		std::unique_lock<std::mutex> mtxlocker(m_counterLock);
		auto result = ++m_counter;
		m_operationStates.Add(result, requestState);
		return result;
	}

	bool AsyncRequestsKeeper::getRequestState(const ics_service::AsyncOperationId _requestId, ics_service::IcsStatusCode::type& _outRequestState)
	{
		return m_operationStates.getValueIfPresent(_requestId, _outRequestState);
	}

	void AsyncRequestsKeeper::setRequestState(const ics_service::AsyncOperationId _requestId, const ics_service::IcsStatusCode::type _newRequestState)
	{
		m_operationStates.setValueIfPresent(_requestId, _newRequestState);
	}

	void AsyncRequestsKeeper::clear()
	{
		m_operationStates.Clear();
	}
}
