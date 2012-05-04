#include "ovasCDriverEnobioApiDll.h"

#include <openvibe-toolkit/ovtk_all.h>

#include <system/Memory.h>
#include <system/Time.h>
#include <boost/thread.hpp>

#include "Enobio.h"

using namespace OpenViBEAcquisitionServer;

namespace
{
	//TODO: shouldn't it be pulled from EnobioApi.dll dynamically
	const OpenViBE::uint32 FREQUENCY_RATE = 250u;

	// Name of the driver
	const char * const DRIVER_NAME = "Enobio API DLL Driver";

	// Helper class that transfer data from Enobio to correct method. It does all casting stuff.
	template <class T, class Callback>
	class Consumer : public IDataConsumer
	{
	public:
		Consumer(Callback& callback)
			: m_callback(callback)
		{ }

		void receiveData(const PData& rData)
		{
			// Just cast and call callback
			m_callback.receiveData(*static_cast<const T*>(rData.getData()));
		}

	private:
		Callback& m_callback;
	};

	// Data used to store information about received sample
	typedef std::vector<OpenViBE::float32> SamplesBuffer;
}

class CDriverEnobioApiDll::CDriverEnobioApiDllPrivate
{
public:
	CDriverEnobioApiDllPrivate(OpenViBEAcquisitionServer::IDriverContext& rDriverContext);
	~CDriverEnobioApiDllPrivate();

	// Method called when enobio data arrived
	void receiveData(const EnobioData& rData);

	// Method called when new status arrived
	void receiveData(const StatusData& rStatus);

	// Open the connection
	OpenViBE::boolean open();

	// Close the connection
	void close();

	// Set number of samples that driver expects
	void setMaxSampleCount(OpenViBE::uint32 ui32SampleCountPerSentBlock);

	// If there is enough samples in buffer, it switches buffers and returns a reference for data
	// that are available for free access until another call of getSamples. If it returns 0, then
	// there is no enough samples in buffer yet
	const SamplesBuffer* getSamples();

private:
	// Helper that forwards data to receiveData(const EnobioData& rData)
	Consumer<EnobioData, CDriverEnobioApiDllPrivate> m_oDataConsumer;

	// Helper that forwards status to receiveData(const StatusData& rStatus)
	Consumer<StatusData, CDriverEnobioApiDllPrivate> m_oStatusConsumer;

	// Enobio driver API
	Enobio m_oEnobio;

	// Driver context for logging
	OpenViBEAcquisitionServer::IDriverContext& m_rDriverContext;

	// Current number of samples in buffer
	OpenViBE::uint32 m_uiSamplesCount;

	// Buffer for samples
	SamplesBuffer m_aSamplesBuffer;

	// Buffer for reader, we swap internals to make if faster
	SamplesBuffer m_aReaderBuffer;

	// Maximum number of samples
	OpenViBE::uint32 m_ui32MaxSamplesCount;

	// Mutex to protect samples
	boost::mutex m_oSamplesMutex;
};

CDriverEnobioApiDll::CDriverEnobioApiDll(IDriverContext& rDriverContext, CommandLineConfiguration& config)
	: IDriver(rDriverContext)
	, m_pCallback(NULL)
	, m_pPrivate(new CDriverEnobioApiDllPrivate(rDriverContext))
{
	m_oHeader.setSamplingFrequency(FREQUENCY_RATE);
	m_oHeader.setChannelCount(EnobioData::NUMBER_OF_CHANNELS);
}

CDriverEnobioApiDll::~CDriverEnobioApiDll()
{
	delete m_pPrivate;
	m_pPrivate = NULL;
	m_pCallback = NULL;
}

const char* CDriverEnobioApiDll::getName(void)
{
	return DRIVER_NAME;
}

OpenViBE::boolean CDriverEnobioApiDll::initialize(const OpenViBE::uint32 ui32SampleCountPerSentBlock,
													 IDriverCallback& rCallback)
{
	if (m_rDriverContext.isConnected())
	{
		return false;
	}

	if (! m_pPrivate->open())
	{
		return false;
	}

	m_pCallback = &rCallback;
	m_pPrivate->setMaxSampleCount(ui32SampleCountPerSentBlock);

	return true;
}


OpenViBE::boolean CDriverEnobioApiDll::start(void)
{
	if (! m_rDriverContext.isConnected())
	{
		return false;
	}
	return ! m_rDriverContext.isStarted();
}

OpenViBE::boolean CDriverEnobioApiDll::loop(void)
{
	if (! m_rDriverContext.isConnected())
	{
		return false; 
	}

	// Check if there are samples available
	const SamplesBuffer* l_paSamples = m_pPrivate->getSamples();
	if (l_paSamples && m_pCallback)
	{
		// We have samples, let's forward them to callback
		if (m_rDriverContext.isStarted())
		{
/*			OpenViBE::float32 a[4] = { 100, 0, 0, 0 };
			for (int i = 0; i <l_paSamples->size() / EnobioData::NUMBER_OF_CHANNELS; ++i)
			{
				a[1] = i;
				m_pCallback->setSamples(a, 1);
			}*/
			m_pCallback->setSamples(l_paSamples->data(), l_paSamples->size() / EnobioData::NUMBER_OF_CHANNELS);
			m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());
		}
	}

	return true;
}

OpenViBE::boolean CDriverEnobioApiDll::stop(void)
{
	if (! m_rDriverContext.isConnected()) 
	{
		return false;
	}
	return m_rDriverContext.isStarted();
}

OpenViBE::boolean CDriverEnobioApiDll::uninitialize(void)
{
	if (! m_rDriverContext.isConnected())
	{
		return false; 
	}
	if (m_rDriverContext.isStarted())
	{
		return false; 
	}

	m_pCallback = NULL;
	m_pPrivate->close();
	return true;
}

OpenViBE::boolean CDriverEnobioApiDll::isConfigurable(void)
{
	return false;
}

OpenViBE::boolean CDriverEnobioApiDll::configure(void)
{
	return false;
}

const OpenViBEAcquisitionServer::IHeader* CDriverEnobioApiDll::getHeader(void)
{
	return &m_oHeader;
}

//
//
//
// -----------------------------------------------------------------------------------------------
//
//
//

CDriverEnobioApiDll::CDriverEnobioApiDllPrivate::CDriverEnobioApiDllPrivate(OpenViBEAcquisitionServer::IDriverContext& rDriverContext)
	: m_oDataConsumer(*this)
	, m_oStatusConsumer(*this)
	, m_rDriverContext(rDriverContext)
	, m_uiSamplesCount(0)
	, m_ui32MaxSamplesCount(0)
{
	// Configure channels for driver
	m_oEnobio.setProperty(Property(Enobio::STR_PROPERTY_ENABLE_CHANNEL_1, EnobioData::NUMBER_OF_CHANNELS > 0));
	m_oEnobio.setProperty(Property(Enobio::STR_PROPERTY_ENABLE_CHANNEL_2, EnobioData::NUMBER_OF_CHANNELS > 1));
	m_oEnobio.setProperty(Property(Enobio::STR_PROPERTY_ENABLE_CHANNEL_3, EnobioData::NUMBER_OF_CHANNELS > 2));
	m_oEnobio.setProperty(Property(Enobio::STR_PROPERTY_ENABLE_CHANNEL_4, EnobioData::NUMBER_OF_CHANNELS > 3));

	m_oEnobio.registerConsumer(Enobio::ENOBIO_DATA, m_oDataConsumer);
	m_oEnobio.registerConsumer(Enobio::STATUS,      m_oStatusConsumer);
}

CDriverEnobioApiDll::CDriverEnobioApiDllPrivate::~CDriverEnobioApiDllPrivate()
{
	m_oEnobio.closeDevice();

	m_oEnobio.deregisterConsumer(Enobio::ENOBIO_DATA, m_oDataConsumer);
	m_oEnobio.registerConsumer(Enobio::STATUS,        m_oStatusConsumer);
}

OpenViBE::boolean CDriverEnobioApiDll::CDriverEnobioApiDllPrivate::open()
{
	if (! m_oEnobio.openDevice())
	{
		m_rDriverContext.getLogManager() << OpenViBE::Kernel::LogLevel_Error << "Could not connect to device\n";
		return false;
	}

	m_oEnobio.startStreaming();

	return true;
}

void CDriverEnobioApiDll::CDriverEnobioApiDllPrivate::close()
{
	m_oEnobio.stopStreaming();
	m_oEnobio.closeDevice();
}

void CDriverEnobioApiDll::CDriverEnobioApiDllPrivate::receiveData(const EnobioData& rData)
{
	// Filter strange stuff
	if (rData.getFlag() != EnobioData::FLAG_NORMAL_DATA)
	{
		return;
	}

	boost::mutex::scoped_lock lock(m_oSamplesMutex);

	// If there is a space in the buffer, add new data
	if (m_uiSamplesCount < m_ui32MaxSamplesCount)
	{
		OpenViBE::int32 l_i32Value = 0;
		// Copy data to buffer
		for (OpenViBE::uint32 i = 0; i < EnobioData::NUMBER_OF_CHANNELS; ++i)
		{
			l_i32Value = rData.getDataFromChannel(0/* number does not start with 1, error in API*/ + i);
			m_aSamplesBuffer[m_uiSamplesCount + i * m_ui32MaxSamplesCount] = OpenViBE::float32(l_i32Value);
		}
/*
		m_rDriverContext.getLogManager() << OpenViBE::Kernel::LogLevel_Info << "C 1 2 3 4 = " << 
			rData.getChannel1() << " " <<
			rData.getChannel2() << " " <<
			rData.getChannel3() << " " <<
			rData.getChannel4() << "\n";
*/
		++m_uiSamplesCount;
	}
}

void CDriverEnobioApiDll::CDriverEnobioApiDllPrivate::receiveData(const StatusData& rData)
{
	//TODO: Different thread issue, fix it
	m_rDriverContext.getLogManager() << OpenViBE::Kernel::LogLevel_Info << "New status arrived " << rData.getCode() << ": " << rData.getString() << "\n";
}

void CDriverEnobioApiDll::CDriverEnobioApiDllPrivate::setMaxSampleCount(OpenViBE::uint32 ui32SampleCountPerSentBlock)
{
	boost::mutex::scoped_lock lock(m_oSamplesMutex);

	m_ui32MaxSamplesCount = ui32SampleCountPerSentBlock;

	// Clear buffer and resize it to new value
	m_uiSamplesCount = 0;
	m_aSamplesBuffer.clear();
	m_aSamplesBuffer.resize(m_ui32MaxSamplesCount * EnobioData::NUMBER_OF_CHANNELS);
	m_aReaderBuffer.resize(m_ui32MaxSamplesCount * EnobioData::NUMBER_OF_CHANNELS);
}

const SamplesBuffer* CDriverEnobioApiDll::CDriverEnobioApiDllPrivate::getSamples()
{
	boost::mutex::scoped_lock lock(m_oSamplesMutex);

	if (m_uiSamplesCount == m_ui32MaxSamplesCount)
	{
		m_aSamplesBuffer.swap(m_aReaderBuffer);
		m_uiSamplesCount = 0;
		return &m_aReaderBuffer;
	}

	return NULL;
}
