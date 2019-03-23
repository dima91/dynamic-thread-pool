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
//#include <iostream>		// FIXME REMOVE ME


namespace dynamicThreadPool {

	using AtomicBoolPointer			= std::shared_ptr<std::atomic<bool>>;
	using DefaultCallback			= std::function<void()>;
	using WorkerCallback			= std::function<void (AtomicBoolPointer)>;


	enum Status {Stopped, Running};



	class WorkerThread {
	private :
		using AfterComputationCallback	= std::function<void (WorkerThread*)>;
		
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
		std::list<WorkerThread*>	freeWorkers;
		std::queue<DefaultCallback>	pendingTasks;
		std::thread					workersManager;
		std::thread					tasksManager;

		std::atomic<int>			activeWorkersCount;
		
		std::mutex				managerMutex;
		std::condition_variable	workersCondition;
		std::condition_variable	tasksCondition;

		std::atomic<Status>		status;

		std::atomic<int> upperLimit;
		std::atomic<int> lowerLimit;


	public:
		DynamicThreadPool	(size_t initialSize=0);
		~DynamicThreadPool	();

		template<typename F, typename... Args>
		auto submit (F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

		void join ();
		void stop ();

		size_t workersCount		();
		size_t freeWorkersCount	();
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
						afterComputation (this);
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

		//std::cout << "Deleted!\n";
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
			workerCondition.notify_one ();
		}
	}


	void WorkerThread::join () {
		if (workerThread.joinable ()) {
			workerThread.join ();
		}
	}


	Status WorkerThread::getStatus () {
		return status;
	}




	/* ================================================== */
	/* ==============   Implementation   ================ */

	DynamicThreadPool::DynamicThreadPool (size_t initialSize) : status (Running) {
		upperLimit						= -1;
		lowerLimit						= 0;
		activeWorkersCount				= initialSize;


		tasksManager	= std::thread ([this] {
			while (status == Running) {
				std::unique_lock<std::mutex> tasksLock (managerMutex);
				tasksCondition.wait (tasksLock, [this] {
					return (pendingTasks.size()>0 && freeWorkers.size()>0) || status==Stopped;
				});

				//std::cout << "Inserted a task!  " << freeWorkers.size () << std::endl;
				if (status != Stopped) {
					DefaultCallback newTask		= std::move (pendingTasks.front());
					pendingTasks.pop ();
					WorkerThread *worker		= std::move (freeWorkers.front());
					freeWorkers.pop_front ();
					worker->assignTask (newTask);
					//std::cout << "Task assigned\n";
				}
			}
		});

		workersManager	= std::thread ([this] {
			while (status == Running) {
				std::unique_lock<std::mutex> workersLock (managerMutex);
				workersCondition.wait (workersLock, [this] {
					//std::cout << "Returning??   " << freeWorkers.size() << std::endl;
					return (pendingTasks.size()>0 && freeWorkers.size()>0) || status==Stopped;
				});
				if (status != Stopped) {
					DefaultCallback newTask		= std::move (pendingTasks.front());
					pendingTasks.pop ();
					WorkerThread *worker		= std::move (freeWorkers.front());
					freeWorkers.pop_front ();
					worker->assignTask (newTask);
				}
			}

			//std::cout << "Destroying workers  " << freeWorkers.size() << std::endl;
			while (activeWorkersCount > 0) {
				std::unique_lock<std::mutex> workersLock (managerMutex);
				while (freeWorkers.size()>0) {
					WorkerThread *worker	= std::move (freeWorkers.front());
					freeWorkers.pop_front ();
					delete (worker);
					activeWorkersCount--;
				}

				//std::cout << "fws: " << freeWorkers.size () << "\tawc: " << activeWorkersCount << std::endl;
			}
		});


		std::function<void (WorkerThread*)> afterCompFun	=
		[this] (WorkerThread *wt) {
			//std::cout << "AfterComputingFunction..\n";
			{
				std::unique_lock<std::mutex> lock (managerMutex);
				freeWorkers.push_back (wt);
			}

			workersCondition.notify_one ();
		};



		for (size_t i=0; i<initialSize; i++) {
			WorkerThread *wt	= new WorkerThread (afterCompFun);
			freeWorkers.emplace_front (wt);
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

		if (status != Running) {
			throw std::runtime_error ("Trying to submit worker on stopped thread pool");
		}

		auto fun	= std::bind (std::forward<F>(f), std::forward<Args>(args)...);
		auto task	= std::make_shared<std::packaged_task<ReturnType()>> (fun);
		auto res	= task->get_future();
		
		{
			std::unique_lock<std::mutex> tLock (managerMutex);			
			pendingTasks.emplace ([task]{(*task)();});
			//std::cout << "New pendingTasks.size : " << pendingTasks.size() << std::endl;
		}

		tasksCondition.notify_one ();

		return res;
	}


	void DynamicThreadPool::stop () {
		status	= Stopped;

		tasksCondition.notify_one ();
		workersCondition.notify_one ();
	}


	void DynamicThreadPool::join () {
		if (tasksManager.joinable ())
			tasksManager.join ();

		if (workersManager.joinable ())
			workersManager.join ();
	}




	size_t DynamicThreadPool::workersCount () {
		return activeWorkersCount;
	}


	size_t DynamicThreadPool::freeWorkersCount () {
		return freeWorkers.size ();
	}


	size_t DynamicThreadPool::tasksCount () {
		return pendingTasks.size ();
	}
	
	
	void DynamicThreadPool::setUpperLimit (size_t uLimit) {
		upperLimit	= uLimit;

		/*std::lock_guard<std::mutex> lock (wMutex);
		while (workers.size() > uLimit) {
			popWorker ();
		}*/
	}


	void DynamicThreadPool::unsetUpperLimit () {
		upperLimit	= -1;
	}




	void DynamicThreadPool::setLowerLimit (size_t lLimit) {
		lowerLimit	= lLimit;
		
		/*std::lock_guard<std::mutex> lock (wMutex);
		while (workers.size() < lLimit) {
			pushNewWorker ();
		}*/
	}




	void DynamicThreadPool::unsetLowerLimit () {
		lowerLimit	= 0;
	}

} // namespace dynamicThreadPool


#endif // DYNAMIC_THREAD_POOL_H