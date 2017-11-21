#pragma once

namespace SGD
{
namespace Job
{
	// focusing on easy to create a job and execute the job
	//	- forcing strong type template
	template <class JobEntryPointType, class DataType>
	class H1JobTemplate
	{
	public:
		// execute the job template
		void Execute()
		{
			EntryPoint(Data);
		}

	protected:
		JobEntryPointType EntryPoint;
		DataType Data;
	};

	// sync object
	class H1JobSyncCounter
	{
	public:
		// counter representing ref-count who refer to this job
		SGD::atomic<uint32> JobCounter;
	};

	// job (task) instance
	template <class JobTemplateType>
	class H1Job
	{
	public:
		H1Job(typename JobTemplateType* Template, H1JobSyncCounter* InSyncCounter)
			: JobTemplate(Template)
			, SyncCounter(InSyncCounter)
		{

		}

		void Execute()
		{
			// sync counter should be zero!
			h1Check(SyncCounter != nullptr);
			h1Check(SyncCounter.JobCounter == 0);

			JobTemplate->Execute();
		}

	protected:
		// it holds the job template
		JobTemplateType* JobTemplate;
		H1JobSyncCounter* SyncCounter;
	};
}
}