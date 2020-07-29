#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include "ConcurrentContainers.h"


template <typename TaskType, template <typename> typename PtrT> class TaskQueueThread
{
	public:
		typedef PtrT<TaskType> TaskPtr;
		TaskQueueThread() { thread = std::thread(ThreadLoop); }

		~TaskQueueThread() 
		{
			AcceptTask(PtrT<StopThreadTask>(new StopThreadTask(stop_flag)));
			thread.join(); 
		};

		inline bool IsFree() const { return !current_task; }

		inline void WaitTaskForFinished() const { while (!IsFree()) Sleep(50); }

		void AcceptTask(const TaskPtr task)
		{
			queue.emplace(task);
			have_task.notify_one();
		}

	private:
		std::thread thread;
		std::mutex task_mtx;
		std::condition_variable have_task;
		concurrent_queue<TaskPtr> queue;  //todo: use priority queue.
		TaskPtr current_task;
		std::atomic_bool stop_flag = false;

		struct StopThreadTask : TaskType
		{
			std::atomic_bool& stop_flag;
			void execute() override { stop_flag = true; }
			StopThreadTask(std::atomic_bool& flg) noexcept : stop_flag(flg) {}
		};

		std::function<void()> ThreadLoop = [&]()
		{
			std::unique_lock<decltype(task_mtx)> task_lock(task_mtx);
			const auto have_task_pred = [this]() { return queue.size(); };
			while (!stop_flag)
			{
				have_task.wait(task_lock, have_task_pred);
				current_task = queue.extract_first();
				try { current_task->execute(); }
				catch (const std::exception&) { /*forward somewhere*/ }
				current_task.reset();
			}
		};

		static inline void Sleep(const std::size_t ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }  // NO!!!!
};