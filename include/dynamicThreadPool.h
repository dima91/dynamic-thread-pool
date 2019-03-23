/**
 * \file dynamicThreadPool.h
 * \brie Header file for DynamicThreadPool class
 * \author Luca Di Mauro
 */

#ifndef DYNAMIC_THREAD_POOL_H
#define DYNAMIC_THREAD_POOL_H


#include <queue>
#include <future>
#include <list>
#include <functional>
#include <memory>
#include <iostream>		// FIXME REMOVE ME


namespace dynamicThreadPool {

	using AtomicBoolPointer			= std::shared_ptr<std::atomic<bool>>;
	using DefaultCallback			= std::function<void()>;
	using WorkerCallback			= std::function<void (AtomicBoolPointer)>;


	enum Status {Stopped, Running};



	class WorkerThread {
	private :
		using AfterComputationCallback	= std::function<void (WorkerThread &)>;
		
		std::thread				workerThread;
		std::mutex				workerMutex;
		std::condition_variable	workerCondition;
		std::atomic<bool>		haltMe;
		std::atomic<bool>		computing;
		std::atomic<Status>		status;

		std::shared_ptr<DefaultCallback>	nextTask;
		AfterComputationCallback			afterComputation;

	public :
		WorkerThread	(AfterComputationCallback afterComp);
		~WorkerThread	();

		// FIXME Chaneg return value to "std::future"
		void assignTask		(DefaultCallback task);
		bool isComputing	();
		void stop			();
		void join			();
		Status	getStatus	();
	};



	class DynamicThreadPool {
	private :
		std::thread taskDispatcher;
		std::list<std::shared_ptr<WorkerThread>> workers;
		std::queue<DefaultCallback>	tasks;

		std::mutex				wMutex;
		std::condition_variable	wCondition;
		std::mutex				tMutex;
		std::condition_variable	tCondition;

		std::atomic<Status>		status;

		std::atomic<int> upperLimit;
		std::atomic<int> lowerLimit;

		void pushNewWorker	();
		void popWorker		();


	public:
		DynamicThreadPool	(size_t initialSize=0);
		~DynamicThreadPool	();

		template<typename F, typename... Args>
		auto submit (F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

		void join ();
		void stop ();

		size_t workersCount		();
		size_t tasksCount		();

		void setUpperLimit		(size_t uLimit);
		void unsetUpperLimit	();
		void setLowerLimit		(size_t lLimit);
		void unsetLowerLimit	();
	};
	



	/* ================================================== */
	/* ==============   Implementation   ================ */

	WorkerThread::WorkerThread (AfterComputationCallback afterComp) : status (Stopped) {
		haltMe		= false;
		computing	= false;
		afterComputation	= std::move (afterComp);

		auto workerFunction	= [this] {
			while (!haltMe) {
				std::unique_lock<std::mutex> lock (workerMutex);
				workerCondition.wait (lock, [&] {return haltMe || nextTask != nullptr;});

				if (!haltMe) {
					try {
						computing	= true;
						(*nextTask) ();
						nextTask	= nullptr;
						computing	= false;
						afterComputation (*this);
					} catch (...) {
						// Some execution returned an error: stopping current WorkerThread!
						haltMe		= true;
					}
				}
			}
		};

		workerThread	= std::thread (workerFunction);
		status			= Running;
	}


	WorkerThread::~WorkerThread () {
		stop ();		
		join ();
	}


	void WorkerThread::assignTask (DefaultCallback task) {
		{
			std::lock_guard<std::mutex> lock (workerMutex);
			nextTask	= std::make_shared<DefaultCallback> (task);
		}
		workerCondition.notify_one ();
	}


	bool WorkerThread::isComputing () {
		return computing;
	}


	void WorkerThread::stop () {
		if (status == Running) {
			haltMe	= true;
			try {
				workerCondition.notify_one ();
			} catch (std::bad_function_call &e) {
				std::cout << "What??\n";
				std::cout << e.what () << std::endl;
			}
		}
	}


	void WorkerThread::join () {
		if (workerThread.joinable ()) {
			std::cout << "Joining..\n";
			workerThread.join ();
			std::cout << "..joined!\n";
		}
	}


	Status WorkerThread::getStatus () {
		return status;
	}




	/* ================================================== */
	/* ==============   Implementation   ================ */

	DynamicThreadPool::DynamicThreadPool (size_t initialSize) : status (Running) {
		upperLimit	= -1;
		lowerLimit	= 0;

		auto dispatcherFun	= [&] () {
			// TODO
			// TODO
			// TODO
			// TODO
			// TODO
			// TODO
			// TODO
		};

		for (size_t i=0; i<initialSize; i++) {
			pushNewWorker ();
		}
	}


	DynamicThreadPool::~DynamicThreadPool () {
		if (status != Stopped)
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
			std::unique_lock<std::mutex> qLock (tMutex);
			if (status != Running)
				throw std::runtime_error("enqueue on stopped ThreadPool");

			tasks.emplace ([task] {(*task)();});
			
			std::unique_lock<std::mutex> wLock (wMutex);
			if (workers.size()<tasks.size() && (((int)workers.size()+1)<upperLimit || upperLimit==-1))
				pushNewWorker ();
		}
		
		tCondition.notify_one ();
		return res;
	}


	void DynamicThreadPool::stop () {
		status	= Stopped;
		tCondition.notify_all ();
	}


	void DynamicThreadPool::join () {
		for (auto &worker : workers)
			if (worker.second->joinable ())
				worker.second->join();
	}




	void DynamicThreadPool::pushNewWorker () {
		AfterC
		WorkerCallback workerCallback	= [this] (AtomicBoolPointer haltMe) {
			while (status==Running && *(haltMe.get())==false) {
				DefaultCallback task;
				{
					std::unique_lock<std::mutex> lock (tMutex);
					tCondition.wait_for (lock, std::chrono::milliseconds (500)),
											[this, haltMe] {return status!=Running || !tasks.empty() ||
															*(haltMe.get())==true;};
					
					if (status != Running || *(haltMe.get())==true)
						break ;
					else {
						task	= std::move (tasks.front());
						tasks.pop ();
					}

					std::unique_lock<std::mutex> wLock (wMutex);
					if (workers.size()>2*tasks.size() && ((int)workers.size()-1)>lowerLimit) {

						wLock.unlock ();
						std::async (std::launch::async, [this] {
							std::this_thread::sleep_for (std::chrono::milliseconds (100));
							std::lock_guard<std::mutex> lock (wMutex);
							popWorker ();
						});
					}
				}

				task();
			}

			std::cout << "EXITING!!!!!!!!\n";
		};
		
		AtomicBoolPointer	haltMe	= std::make_shared<std::atomic<bool>> (false);
		SharedThread		sharedT	= std::make_shared<std::thread> (workerCallback, haltMe); 
		workers.emplace_front (std::make_pair<AtomicBoolPointer&, SharedThread&> (haltMe, sharedT));
	}




	void DynamicThreadPool::popWorker () {
		auto worker		= std::move (workers.back());
		workers.pop_back ();
		*(worker.first)	= true;
		tCondition.notify_all ();

		if (worker.second->joinable()) {
			//std::cout << "Joining\n";
			qCondition.notify_all ();
			worker.second->join ();		// FIXME It is blocking for ever!!!! (only I try to decrease thread pool from worker thread)
			//std::cout << "Joined\n";
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