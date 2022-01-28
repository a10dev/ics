

#pragma once

#include <boost/noncopyable.hpp>

#include "../../../../thrift/cpp/ics/ics_types.h"
#include "../../thsMap.h"

namespace ics
{
	// Содержит историю с информацией о состоянии выполнения асинхронных процедур.
	typedef ThsMap<ics_service::AsyncOperationId, ics_service::IcsStatusCode::type> AsyncOperationTable;

	// Хранитель информации о статусе выполнения асинхронных процедур.
	class AsyncRequestsKeeper: private boost::noncopyable
	{
	public:

		AsyncRequestsKeeper();

		// Возвращает истину в том случае, если такой запрос с указанным номером существует (существовал).
		bool isRequestPresent(const ics_service::AsyncOperationId _requestId) const;

		// Создает запрос на выполнение новой асинхронной процедуры.
		ics_service::AsyncOperationId createNewRequest(ics_service::IcsStatusCode::type requestState = ics_service::IcsStatusCode::Ics_NotFinishedYet);

		// Удаляет все завершенные запросы.
		void removeFinishedRequests();

		// Возвращает статус выполнения асинхронной операции. Если такого запрорса не существует, метод вернут false.
		bool getRequestState(const ics_service::AsyncOperationId _requestId, ics_service::IcsStatusCode::type& _outRequestState) /* const */;

		// Обновляет статус выполнения асинхронной процедуры.
		void setRequestState(const ics_service::AsyncOperationId _requestId, const ics_service::IcsStatusCode::type _newRequestState);

		// Очищает очередь запросов, но не обнуляет счётчик запросов, что позволяет отсеивать (игнорировать) старые запросы и создавать
		// новые-оригинальные запросы, с уникальными идентификаторами.
		void clear();

	private:
		AsyncOperationTable m_operationStates;
		ics_service::AsyncOperationId m_counter;
		std::mutex m_counterLock;
	};
}
