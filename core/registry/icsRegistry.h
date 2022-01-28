

#pragma once

#include "../../../../../thrift/cpp/ics/ics_types.h"
#include "registryKeyDescription.h"
#include <map>
#include "../../../helpers.h"
#include <boost/noncopyable.hpp>
#include "../icsSettings.h"
#include "hasherCrc32.h"
#include "hasherMd5.h"
#include "hasherGost94.h"
#include "hasherGost2012_256.h"
#include "hasherGost2012_512.h"
#include "controlledKeysStorage.h"
#include "../../../arm_events_client/event_emiter.h"


namespace ics
{
	namespace registry
	{
		typedef std::map<ics_service::HashAlg::type, Hasher*> HashersTable;
		typedef std::vector<ics::registry::RegistryKeyDescription> KeyList;
		typedef std::set<std::wstring> StrSet;

		class RegControl : private boost::noncopyable
		{
		public:

			RegControl(logfile& _log);

			~RegControl();

			// Возвращает информацию состоянии целостности.
			bool isCompromised() const;

			// Добавляет в область контроля целостности ключ реестра.
			bool addKey(std::wstring _keypath, bool _includeSubkeys, ics_service::HashAlg::type _hashAlgorithm, ics_service::IcsStatusCode::type& result);

			// Удаляет из области контроля целостности ключ реестра.
			bool removeKey(std::wstring _keypath);

			// Пересчитывает контрольную сумму для ранее добавленного ключа реестра по-заданному алгоритму хеширования.
			bool changeHashAlgorithm(std::wstring _keypath, ics_service::HashAlg::type _hashAlg, ics_service::IcsStatusCode::type& result);

			// Пересчитывает хеш для ранее добавленного ключа реестра, если новая сумма отличается от старой
			// то об этом можно узнать в _isChanged.
			bool recalculate(std::wstring _keypath, bool& _isChanged, ics_service::IcsStatusCode::type& result);

			// Пересчитывает контрольные суммы для всей области контроля целостности реестра.
			bool recalculate();

			// Возвращает список контроллируемых ключей.
			void getListKeys(KeyMap& _outKeys) const;

			// Возвращает список ключей, с нарушенной целостностью.
			void getListChangedKeys(KeyMap& _outKeys) const;

			// Возвращает истину если имеются объекты с нарушеной целостностью.
			bool hasChangedObjects() const;
			StrSet getMissedKeys() const;

			void flushData()
			{
				m_storage.flush();
			}

			// Возвращает список доступных алгоритмов хэширования
			std::vector<ics_service::HashAlg::type> getHashesList();

		private:
			logfile& m_log;

			HasherCrc32 m_hasherCrc32;
			HasherMd5 m_hasherMd5;
			HasherGost94 m_hasherGost94;
			HasherGost2012_256 m_hasherGost2012_256;
			HasherGost2012_512 m_hasherGost2012_512;
			HashersTable m_hashers;

			Storage m_storage;

		protected:
			bool calc(const std::wstring& _keypath, bool _includeSubkeys, ics_service::HashAlg::type _algType, RegistryKeyDescription& _outInfo, ics_service::IcsStatusCode::type& result);

			// Изменить алгоритм по которому считается целостность объекта и сохранить в БД информацию.
			bool hlpChangeHashAlgorithm(std::wstring _keypath, ics_service::HashAlg::type _hashAlg, ics_service::IcsStatusCode::type& result);
			void markAsChanged(RegistryKeyDescription _keyInfo);
			void markAsNormal(std::wstring _key);
			void markAsMissed(std::wstring _key);

			std::shared_ptr<events_client::EventEmiter> getEventEmiter();
		};
	}
}
