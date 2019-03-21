/**
 * \file dynamicThreadPool.h
 * \brie Header file for DynamicThreadPool class
 * \author Luca Di Mauro
 */

#ifndef DYNAMIC_THREAD_POOL_H
#define DYNAMIC_THREAD_POOL_H


#include <queue>
#include <iostream>		// REMOVE ME
#include <future>
#include <vector>
#include <functional>


namespace dynamicThreadPool {

	using AtomicBoolPointer	= std::shared_ptr<std::atomic<bool>>;
	using DefaultCallback	= std::function<void()>;
	using WorkerCallback	= std::function<void (AtomicBoolPointer)>;
	using SharedThread		= std::shared_ptr<std::thread>;


	enum ThreadPoolStatus {Stopped, Running, Stopping};


	class DynamicThreadPool {
	private :
		//std::vector<std::thread>	workers;
		std::vector<std::pair<AtomicBoolPointer, SharedThread>> workers;
		std::queue<DefaultCallback>	tasks;

		std::mutex				wMutex;
		std::mutex				qMutex;
		std::condition_variable	qCondition;

		std::atomic<ThreadPoolStatus>	status;

		std::atomic<int> upperLimit;
		std::atomic<int> lowerLimit;


	public:
		DynamicThreadPool	(size_t initialSize=0);
		~DynamicThreadPool	();

		template<typename F, typename... Args>
		auto submit (F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

		void join ();
		void stop ();

		void pushNewWorker	();
		void popWorker		();

		size_t size 			();
		size_t tasksCount		();
		void setUpperLimit		(size_t uLimit);
		void unsetUpperLimit	();
		void setLowerLimit		(size_t lLimit);
		void unsetLowerLimit	();
	};
	



	/* ================================================== */
	/* ==============   Implementation   ================ */

	DynamicThreadPool::DynamicThreadPool (size_t initialSize) : status (Running) {
		upperLimit	= -1;
		lowerLimit	= 0;

		for (size_t i=0; i<initialSize; i++) {
			pushNewWorker ();
		}
	}


	DynamicThreadPool::~DynamicThreadPool () {
		if (status != Stopping)
			stop ();

		join ();
	}


	template<class F, class... Args>
	auto DynamicThreadPool::submit (F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
		using ReturnType	= decltype(f(args...));

		auto fun	= std::bind (std::forward<F>(f), std::forward<Args>(args)...);
		auto task	= std::make_shared<std::packaged_task<ReturnType()>> (fun);
		auto res	= task->get_future();
		
		{
			std::unique_lock<std::mutex> qLock (qMutex);
			if (status != Running)
				throw std::runtime_error("enqueue on stopped ThreadPool");

			tasks.emplace ([task] {(*task)();});
			
			std::unique_lock<std::mutex> wLock (wMutex);
			if (workers.size()<tasks.size() && (((int)workers.size()+1)<upperLimit || upperLimit==-1))
				pushNewWorker ();
		}
		
		qCondition.notify_one ();
		return res;
	}


	void DynamicThreadPool::stop () {
		status	= Stopping;
		qCondition.notify_all ();
		status	= Stopped;
	}


	void DynamicThreadPool::join () {
		for (auto &worker : workers)
			if (worker.second->joinable ())
				worker.second->join();
	}




	void DynamicThreadPool::pushNewWorker () {
		WorkerCallback workerCallback	= [this] (AtomicBoolPointer haltMe) {
			while (status==Running && *(haltMe.get())==false) {
				DefaultCallback task;
				{
					std::unique_lock<std::mutex> lock (qMutex);
					qCondition.wait (lock, [this, haltMe] {std::cout<<"hm: "<<*(haltMe.get())<<"\n"; return status!=Running || !tasks.empty() ||
																	*(haltMe.get())==true;});
					
					if (status != Running || *(haltMe.get())==true)
						break ;
					else {
						task	= std::move (tasks.front());
						tasks.pop ();
					}

					/*std::unique_lock<std::mutex> wLock (wMutex);
					if (workers.size()>2*tasks.size() && ((int)workers.size()-1)>lowerLimit) {
						std::cout << "Decreasing..\n";
						popWorker ();
						std::cout << "Decreased..\n";
					}*/
				}

				task();
			}
		};
		
		AtomicBoolPointer	haltMe	= std::make_shared<std::atomic<bool>> (false);
		SharedThread		sharedT	= std::make_shared<std::thread> (workerCallback, haltMe); 
		workers.emplace_back (std::make_pair<AtomicBoolPointer&, SharedThread&> (haltMe, sharedT));
	}




	void DynamicThreadPool::popWorker () {
		auto worker		= std::move (workers.back());
		workers.pop_back ();
		*(worker.first)	= true;
		qCondition.notify_all ();

		if (worker.second->joinable()) {
			worker.second->join ();		// FIXME It is blocking for ever!!!! (only I try to decrease thread pool from worker thread)
		}
	}




	size_t DynamicThreadPool::size () {
		return workers.size ();
	}


	size_t DynamicThreadPool::tasksCount () {
		return tasks.size ();
	}
	
	
	void DynamicThreadPool::setUpperLimit (size_t uLimit) {
		upperLimit	= uLimit;

		std::lock_guard<std::mutex> lock (wMutex);
		while (workers.size() > uLimit) {
			popWorker ();
		}
	}


	void DynamicThreadPool::unsetUpperLimit () {
		upperLimit	= -1;
	}




	void DynamicThreadPool::setLowerLimit (size_t lLimit) {
		lowerLimit	= lLimit;
		
		std::lock_guard<std::mutex> lock (wMutex);
		while (workers.size() < lLimit) {
			pushNewWorker ();
		}
	}




	void DynamicThreadPool::unsetLowerLimit () {
		lowerLimit	= 0;
	}

} // namespace dynamicThreadPool


#endif // DYNAMIC_THREAD_POOL_H