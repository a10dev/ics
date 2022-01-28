

#pragma once

#include <boost/noncopyable.hpp>

#include "../../../../thrift/cpp/ics/ics_types.h"
#include "../../thsMap.h"

namespace ics
{
	// �������� ������� � ����������� � ��������� ���������� ����������� ��������.
	typedef ThsMap<ics_service::AsyncOperationId, ics_service::IcsStatusCode::type> AsyncOperationTable;

	// ��������� ���������� � ������� ���������� ����������� ��������.
	class AsyncRequestsKeeper: private boost::noncopyable
	{
	public:

		AsyncRequestsKeeper();

		// ���������� ������ � ��� ������, ���� ����� ������ � ��������� ������� ���������� (�����������).
		bool isRequestPresent(const ics_service::AsyncOperationId _requestId) const;

		// ������� ������ �� ���������� ����� ����������� ���������.
		ics_service::AsyncOperationId createNewRequest(ics_service::IcsStatusCode::type requestState = ics_service::IcsStatusCode::Ics_NotFinishedYet);

		// ������� ��� ����������� �������.
		void removeFinishedRequests();

		// ���������� ������ ���������� ����������� ��������. ���� ������ �������� �� ����������, ����� ������ false.
		bool getRequestState(const ics_service::AsyncOperationId _requestId, ics_service::IcsStatusCode::type& _outRequestState) /* const */;

		// ��������� ������ ���������� ����������� ���������.
		void setRequestState(const ics_service::AsyncOperationId _requestId, const ics_service::IcsStatusCode::type _newRequestState);

		// ������� ������� ��������, �� �� �������� ������� ��������, ��� ��������� ��������� (������������) ������ ������� � ���������
		// �����-������������ �������, � ����������� ����������������.
		void clear();

	private:
		AsyncOperationTable m_operationStates;
		ics_service::AsyncOperationId m_counter;
		std::mutex m_counterLock;
	};
}
