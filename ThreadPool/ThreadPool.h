#include <thread>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <queue>
//#include <chrono>

template <typename Executable, template <typename> typename FancyPtrT> class ThreadPool
{
	typedef FancyPtrT<Executable> ExecutablePtr;
	
	class Task  // decided to leave it nested
	{
		public:
			typedef std::size_t IdType;

			Task(const ExecutablePtr& _exec = nullptr, const IdType _id = 0, const std::size_t _prior = 2) :
				executable(_exec),
				id(_id),
				priority(_prior)
			{}

			//Task() {}

			inline const ExecutablePtr& GetExecutable() const { return executable; }

			inline IdType GetId() const { return id; }

			inline bool operator< (const Task& other) const { return priority < other.priority; }

		private:

			std::size_t priority;
			IdType id;
			ExecutablePtr executable;
	};

	class Thread  // and this one too
	{
		public:
			Thread() { std::thread(ThreadLoop).detach(); }

			inline bool IsFree() const { return !executable_ptr; }

			inline void WaitTaskForFinished() const { while (!IsFree()) Sleep(); }

			void AcceptTask(const ExecutablePtr&& _task) { executable_ptr = _task; }

		private:
			ExecutablePtr executable_ptr;

			std::function<void()> ThreadLoop = [&]()
			{
				while (true)
					if (executable_ptr)
						executable_ptr->execute(),
						executable_ptr.reset();
					else Sleep();
			};

			//const Task& GetLastTask() const { return task; }
	};


	typedef typename Task::IdType TaskIdType;
	std::mutex queue_mutex;
	std::priority_queue<Task> task_queue;
	TaskIdType tasks_accepted = 0;
	std::unordered_map<TaskIdType, Thread*> task_assignment_map;

	static inline void Sleep() { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }

	const std::function<void(const std::size_t)> main_loop = [&](const std::size_t pool_capacity)
	{
		std::vector<Thread> thread_pool(pool_capacity);
		std::size_t free_threads_available = pool_capacity,
					tasks_available, i;

		std::vector<Thread*> thread_ptrs;  // Pointers to the left of {free_threads_available} are free threads and to the right to this value are currently working ones.
		std::unordered_map<Thread*, TaskIdType> thread_assignment_map; // Чтобы не тащить ид задачи в поток или не рисовать свой бимап.
		thread_ptrs.reserve(pool_capacity);
		thread_assignment_map.reserve(pool_capacity);
		for (Thread& thread : thread_pool)
			thread_ptrs.emplace_back(&thread),
			thread_assignment_map.emplace(&thread, 0);

		Thread *thread;
		const Task* task;
		TaskIdType task_id;
		while (true)
			if ((tasks_available = task_queue.size()) && free_threads_available)
			{
				queue_mutex.lock();
				for (i = tasks_available > free_threads_available ? free_threads_available : tasks_available; i--;)
					thread = thread_ptrs[--free_threads_available],
					task = &task_queue.top(),
					task_id = task->GetId(),
					task_assignment_map[task_id] = thread,
					thread_assignment_map[thread] = task_id,
					thread->AcceptTask(std::move(task->GetExecutable())),
					task_queue.pop();
				queue_mutex.unlock();
			}
			else
			{
				Sleep();
				if (free_threads_available < pool_capacity)
				{
					queue_mutex.lock();
					for (i = free_threads_available; i < pool_capacity; ++i)
						if (thread_ptrs[i]->IsFree())
							std::swap(thread_ptrs[i], thread_ptrs[free_threads_available++]),
							task_assignment_map.erase(thread_assignment_map[thread]);
					queue_mutex.unlock();
				}
			}
	};

public:
	ThreadPool(const std::size_t max_threads = std::thread::hardware_concurrency())
	{
		std::thread(main_loop, max_threads).detach();
	}

	TaskIdType AddTask(const std::shared_ptr<Executable>& executable)
	{
		queue_mutex.lock();
		task_queue.emplace(executable, tasks_accepted);
		task_assignment_map[tasks_accepted] = nullptr;
		queue_mutex.unlock();
		return tasks_accepted++;
	}

	inline bool TaskIsDone(TaskIdType task_id) const
	{
		return !task_assignment_map.count(task_id) && task_id < tasks_accepted;
	}

	void WaitTaskForFinished(TaskIdType task_id) const
	{
		while (!TaskIsDone(task_id)) Sleep();
	}
};
