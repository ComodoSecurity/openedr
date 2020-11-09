//
// edrav2.libcore
// 
// Author: Denis Bogdanov (26.02.2019)
// Reviewer: Yury Podpruzhnikov (13.03.2019)
//
///
/// @file Data generator class declaration
///
/// @addtogroup dataGenerators
/// @{
///
#pragma once
#include <objects.h>
#include <command.hpp>
#include <common.hpp>
#include <threadpool.hpp>

namespace openEdr {

///
/// This class can generate data according to specified template.
///
class DataGenerator : public ObjectBase<CLSID_DataGenerator>,
	public ICommandProcessor,
	public IDataProvider,
	public IDataGenerator,
	public IInformationProvider,
	public IProgressReporter
{
private:
	ObjPtr<IDataReceiver> m_pReceiver;
	ThreadPool m_threadPool{ "DataGenerator::WorkingPool" };
	ThreadPool::TimerContextPtr m_pTimerCtx;
	Dictionary m_vData;
	Size m_nPeriod = 0;

	std::mutex m_mtxData;
	bool m_fStop = true;
	bool m_fAutoStop = false;
	Size m_nMaxCount = 0;
	Size m_nCurrCount = 0;
	Size m_nTotalCount = 0;
	Sequence m_seqCurrData;
	ObjPtr<IDataProvider> m_pCurrProvider;

	bool asyncSendData();

protected:
	
	//
	//
	//
	virtual ~DataGenerator() override;

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - the object's configuration including the following fields:
	///   **data** - data template (dictionary or sequence of dictionaries).
	///   **period** - timeout in milliseconds for sending data.
	///   **receiver** - object for pushing data.
	///   **count** - count of generated data.
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	void finalConstruct(Variant vConfig);

	// IDataGenerator
	
	/// @copydoc IDataGenerator::start()
	virtual void start(std::string_view sName, Size nCount = c_nMaxSize) override;
	/// @copydoc IDataGenerator::stop()
	virtual void stop() override;
	/// @copydoc IDataGenerator::terminate()
	virtual void terminate() override;

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	virtual Variant execute(Variant vCommand, Variant vParams) override;

	// IDataProvider

	/// @copydoc IDataProvider::get()
	virtual std::optional<Variant> get() override;

	// IInformationProvider

	/// @copydoc IInformationProvider::getInfo()
	virtual Variant getInfo() override;

	///
	/// Gets progress information.
	///
	/// @return The function returns a data packet with information.
	///
	virtual Variant getProgressInfo() override;

};

//
//
//
class RandomDataGenerator : public ObjectBase<CLSID_RandomDataGenerator>,
	public ICommandProcessor
{
private:
	std::atomic<Size> m_nStartValue;
	std::random_device m_randDevice;
	std::mt19937 m_fnRandGen;

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - the object's configuration including the following fields:
	///   **start** - start value for getNext() command.
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	void finalConstruct(Variant vConfig);

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	virtual Variant execute(Variant vCommand, Variant vParams = Variant()) override;
};

} // namespace openEdr
/// @} 
