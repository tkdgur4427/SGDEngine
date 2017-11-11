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

	// job (task) instance
	template <class JobTemplateType>
	class H1Job
	{
	public:
		H1Job(typename JobTemplateType* Template)
			: JobTemplate(Template)
		{

		}

		void Execute()
		{
			JobTemplate->Execute();
		}

	protected:
		// it holds the job template
		JobTemplateType* JobTemplate;
	};

	// sync object
	class H1JobSyncCounter
	{
	public:
		SGD::atomic<uint32> JobCounter;
	};
}
}