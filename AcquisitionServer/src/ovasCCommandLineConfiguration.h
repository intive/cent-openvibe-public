#ifndef __OpenViBE_AcquisitionServer_CCommandLineConfiguration_H__
#define __OpenViBE_AcquisitionServer_CCommandLineConfiguration_H__

#include <string>

namespace OpenViBEAcquisitionServer
{
	class CommandLineConfiguration
	{
		public:
			static CommandLineConfiguration buildFromArgs(int argc, char** argv);
			static void usageOutput();
			bool isTraceEnabled();
			int getListenPort();
			int getEnobioPort();
			std::string getEnobioHost();
			bool isEnobioApiDriverEnabled();
			bool isOscilatorEnabled();

			~CommandLineConfiguration();

		private:
			CommandLineConfiguration();
			static void handleFlagParameter(CommandLineConfiguration& config, std::string flag);
			static void handleValueParameter(CommandLineConfiguration& config, std::string parameter, std::string value);
			static bool startsWith(std::string value, std::string prefix);

			bool        m_traceEnabled;
			int         m_listenPort;
			int         m_enobioPort;
			std::string m_enobioHost;
			bool        m_useEnobioApiDriver;
			bool        m_useOscilator;
	};
};
#endif