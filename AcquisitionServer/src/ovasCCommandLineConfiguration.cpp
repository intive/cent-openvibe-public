#include "ovasCCommandLineConfiguration.h" 

#include <iostream>
#include <sstream>

using namespace OpenViBEAcquisitionServer;

namespace 
{
	template<class T>
	T fromString(const std::string& s)
	{
		 std::istringstream stream (s);
		 T t;
		 stream >> t;
		 return t;
	}

	const std::string flagParameter               = "--";
	const std::string enableTraceParameter        = "--trace";
	const std::string useEnobioApiDriverParameter = "--use-enobio-api-driver";
	const std::string useOscilatorParameter       = "--use-oscilator-as-source";

	const std::string valueParameter              = "-";
	const std::string listenPortParameter         = "-port";
	const std::string enobioHostParameter         = "-enobio-host";
	const std::string enobioPortParameter         = "-enobio-port";
	const std::string helpParameter               = "-h";

	const int defaultListenPort         = 1024;
	const int defaultEnobioPort         = 1234;
	const std::string defaultEnobioHost = "localhost";
}

CommandLineConfiguration::CommandLineConfiguration()
	: m_traceEnabled(false)
	, m_listenPort(defaultListenPort)
	, m_enobioHost(defaultEnobioHost)
	, m_enobioPort(defaultEnobioPort)
	, m_useEnobioApiDriver(false)
	, m_useOscilator(false)
{
}

CommandLineConfiguration::~CommandLineConfiguration()
{
}

CommandLineConfiguration CommandLineConfiguration::buildFromArgs(int argc, char** argv)
{
	CommandLineConfiguration config;

	for (int i = 1; i < argc && argv; ++i) // starting from 1 to omit binary name
	{
		std::string argument(argv[i]);
		if (startsWith(argument, helpParameter))
		{
			usageOutput();
			exit(0);
		}
		if (startsWith(argument, flagParameter))
		{
			handleFlagParameter(config, argument);
		}
		else if (startsWith(argument, valueParameter))
		{
			// Not enough parameters
			if (i + 1 >= argc)
			{
				usageOutput();
				exit(0);
			}

			std::string value(argv[++i]);
			handleValueParameter(config, argument, value);
		}
	}
	return config;
}

void CommandLineConfiguration::usageOutput()
{
	std::cout << "Usage: OpenVibe-acquisition-server-dynamic.exe "
			<< "[" << helpParameter << " | "
			<< "[" << enableTraceParameter << "] "
			<< "[" << useEnobioApiDriverParameter << "] "
			<< "[" << listenPortParameter << " <port>] "
			<< "[" << enobioHostParameter << " <enobio-host>] "
			<< "[" << enobioPortParameter << " <enobio-port>]" << std::endl 

			<< "\t" << helpParameter << "\t\t- displays this help message" << std::endl 
			<< "\t" << enableTraceParameter << "\t\t- sets logging level to trace" << std::endl
			<< "\t" << useEnobioApiDriverParameter << "\t\t- uses API based driver" << std::endl
			<< "\t" << listenPortParameter << "\t\t- sets port, on which acquisition server listens for incoming connections "
				<< "(like from OpenViBE Designer) to <port>" << std::endl
			<< "\t" << enobioHostParameter << "\t- sets host, to which starlab enobio device is connected to <enobio-host>" << std::endl
			<< "\t" << enobioPortParameter << "\t- sets port, on which starlab enobio's device driver listens for incoming "
			<< "connections to <enobio-port>" << std::endl;
}

bool CommandLineConfiguration::isTraceEnabled()
{
	return m_traceEnabled;
}

int CommandLineConfiguration::getListenPort()
{
	return m_listenPort;
}

int CommandLineConfiguration::getEnobioPort()
{
	return m_enobioPort;
}

std::string CommandLineConfiguration::getEnobioHost()
{
	return m_enobioHost;
}

bool CommandLineConfiguration::isEnobioApiDriverEnabled()
{
	return m_useEnobioApiDriver;
}

bool CommandLineConfiguration::isOscilatorEnabled()
{
	return m_useOscilator;
}

void CommandLineConfiguration::handleFlagParameter(CommandLineConfiguration& config, std::string flag)
{
	if (startsWith(flag, enableTraceParameter))
	{
		config.m_traceEnabled = true;
		return;
	}

	if (startsWith(flag, useEnobioApiDriverParameter))
	{
		config.m_useEnobioApiDriver = true;
		return;
	}

	if (startsWith(flag, useOscilatorParameter))
	{
		config.m_useOscilator = true;
		return;
	}

}

void CommandLineConfiguration::handleValueParameter(CommandLineConfiguration& config, std::string parameter, std::string value)
{
	if (startsWith(parameter, listenPortParameter))
	{
		config.m_listenPort = fromString<int>(value);
		return;
	}
	if (startsWith(parameter, enobioPortParameter))
	{
		config.m_enobioPort = fromString<int>(value);
		return;
	}
	if (startsWith(parameter, enobioHostParameter))
	{
		config.m_enobioHost = value;
		return;
	}
}

bool CommandLineConfiguration::startsWith(std::string value, std::string prefix)
{
	return !value.compare(0, prefix.size(), prefix);
}