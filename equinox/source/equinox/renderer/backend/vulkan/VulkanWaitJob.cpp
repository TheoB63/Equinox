#include "eqnpch.h"
#include "VulkanWaitJob.h"
#include "TimelineSemaphore.h"
#include "equinox/core/JobSystem.h"
#include "equinox/core/Log.h"
#include <thread>

namespace Equinox
{
	struct WaitJobData
	{
		TimelineSemaphore* semaphore;
		u64 targetValue;
		JobSystem::Counter* counter;
	};

	static void WaitJobFunction(JobSystem::JobArgs args)
	{
		WaitJobData* data = (WaitJobData*)args.data;

		u64 currentValue = data->semaphore->GetValue();

		if (currentValue >= data->targetValue)
		{
			// Condition met!            
			delete data;
		}
		else
		{
			// Condition NOT met.
			// Yield to avoid hammering the JobSystem lock and starving other threads
			std::this_thread::yield();

			// Re-queue the job to run again later.
			JobSystem::Execute(WaitJobFunction, data, data->counter);
		}
	}

	void VulkanWaitJob::Dispatch(TimelineSemaphore& semaphore, u64 targetValue, JobSystem::Counter* counter)
	{
		WaitJobData* data = new WaitJobData{ &semaphore, targetValue, counter };

		// Standard dispatch. Execute increments the counter.
		JobSystem::Execute(WaitJobFunction, data, counter);
	}
}