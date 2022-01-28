

#include "registry.h"

#include "../core/registry/icsRegistry.h"

#define print_result(x) log.print(std::string(__FUNCTION__) + (x == true ? " success" :" failed"))

namespace ics
{
	namespace registry
	{
		const char* testKey = "HKEY_LOCAL_MACHINE\\SOFTWARE\\xxxx";
		const char* testKey2 = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Realtek";

		void doTest()
		{
			bool success = false;
			logfile log("tests_regcontrol.log");
			log.print("\n\n");

			ics::Settings settings;

			RegControl control(log);

			for (int i = 0; i < 2; ++i)
			{
				log.print(std::string(__FUNCTION__) + " add key to control area: " + testKey);
				ics_service::IcsStatusCode::type result;
				bool success = control.addKey(strings::s_ws(testKey), true, ics_service::HashAlg::Md_5, result);
				print_result(success);
				log.print(std::string(__FUNCTION__) + " Again try to add early added key to control area.");
			}

			log.print("Change content of the key: " + std::string(testKey));
			getchar();

			control.recalculate();
			ics::registry::KeyMap changed;
			control.getListChangedKeys(changed);
			for (auto i : changed)
			{
				log.print("\tChanged:\t" + strings::ws_s(i.first));
			}

			log.print(std::string(__FUNCTION__) + " remove key from control area.");
			success = control.removeKey(strings::s_ws(testKey));
			print_result(success);
		}
	}
}

