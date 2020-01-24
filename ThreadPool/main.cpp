#include "IExecutable.h"
#include "ThreadPool.h"

#include <iostream>
#include <memory>





typedef ThreadPool<IExecutable, std::shared_ptr> StandardThreadPool;

static StandardThreadPool global_thread_pool;



void func1(int arg)
{
	std::cout << "Called func1 with arg = " << arg << " in thread " << std::this_thread::get_id() << '\n';
}


void func2(float arg1, int arg2)
{
	std::cout << "Called func2 with arg1 = " << arg1 << ", arg2 = " << arg2 << " in thread " << std::this_thread::get_id() << '\n';
}


void func3(bool arg)
{
	std::cout << "Called func3 with arg = " << arg << " in thread " << std::this_thread::get_id() << '\n';
	std::this_thread::sleep_for(std::chrono::seconds(5));
}

class Task1 : public IExecutableT<int>
{

	void execute() override
	{
		std::cout << "Invoking Task1\n";
		func1(std::get<0>(args));
		std::cout << "Task1 is done\n";
	}
public:
	using IExecutableT::IExecutableT;
	virtual ~Task1() { std::cout << "Task1 destructed\n"; }
};


class Task2 : public IExecutableT<float, int>
{
	void execute() override
	{
		std::cout << "Executing Task2\n";
		func2(std::get<0>(args), std::get<1>(args));
	}
public:
	using IExecutableT::IExecutableT;
};


class Task3 : public IExecutableT<bool>
{
	void execute() override
	{
		std::cout << "Executing Task3\n";
		func3(std::get<0>(args));
	}
public:
	using IExecutableT::IExecutableT;
};


int main()
{
	int user_input = 1;
	std::size_t task_id = 0;
	while (user_input && std::cin >> user_input)
	{
		switch (user_input)
		{
			case 1: task_id = global_thread_pool.AddTask(std::shared_ptr<Task1>(new Task1(int(user_input)))); break;
			case 2: task_id = global_thread_pool.AddTask(std::shared_ptr<Task2>(new Task2(43.321f, 70))); break;
			case 3: task_id = global_thread_pool.AddTask(std::shared_ptr<Task3>(new Task3(false))); break;
		}
		global_thread_pool.WaitTaskForFinished(task_id);
	}
	for (std::size_t i = 1; i <= 20000; ++i)
		global_thread_pool.AddTask(std::shared_ptr<Task1>(new Task1(std::move(i))));
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	global_thread_pool.WaitTaskForFinished(30000);
	global_thread_pool.WaitTaskForFinished(1500);
	std::this_thread::sleep_for(std::chrono::seconds(100));
	return 0;
}


