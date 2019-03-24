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

		void combineFirstWorkerWithFirstTask ();
		void createFreeWorker ();
		void destroyFirstFreeWorker ();
		void resize ();


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
		activeWorkersCount				= 0;


		tasksManager	= std::thread ([this] {
			while (status == Running) {
				std::unique_lock<std::mutex> tasksLock (managerMutex);
				tasksCondition.wait (tasksLock, [this] {
					return	status==Stopped
							|| (pendingTasks.size()>0 && freeWorkers.size()==0 && (activeWorkersCount<upperLimit || upperLimit == -1))	// I can create a worker
							|| (pendingTasks.size()>0 && freeWorkers.size()>0);															// I can assign a task
				});

				if (status != Stopped) {
					//Checking if I can create a worker
					if (pendingTasks.size()>0 && freeWorkers.size()==0 && (activeWorkersCount<upperLimit || upperLimit == -1)) {
						createFreeWorker ();
					}
					else if (pendingTasks.size()>0 && freeWorkers.size()>0) {
						combineFirstWorkerWithFirstTask ();
					}
				}
			}
		});

		workersManager	= std::thread ([this] {
			while (status == Running) {
				std::unique_lock<std::mutex> workersLock (managerMutex);
				workersCondition.wait (workersLock, [this] {
					return	(freeWorkers.size()>0 && pendingTasks.size()>0)									// I can assign a task
							|| (freeWorkers.size()>0 && activeWorkersCount>upperLimit && upperLimit != -1)	// I haveto destroy a worker
							|| status==Stopped;
				});
	
				if (status != Stopped) {
					//Checking if upperLimit is exceeded
					if (activeWorkersCount>upperLimit && upperLimit != -1) {
						destroyFirstFreeWorker ();
					}
					else if (pendingTasks.size()>0)
						combineFirstWorkerWithFirstTask ();
				}
			}

			while (activeWorkersCount > 0) {
				std::unique_lock<std::mutex> workersLock (managerMutex);
				while (freeWorkers.size()>0) {
					destroyFirstFreeWorker ();
				}
			}
		});


		for (size_t i=0; i<initialSize; i++) {
			createFreeWorker ();
		}
	}


	DynamicThreadPool::~DynamicThreadPool () {
		if (status != Stopped)
			stop ();

		join ();
	}


	void DynamicThreadPool::combineFirstWorkerWithFirstTask () {
		DefaultCallback newTask		= std::move (pendingTasks.front());
		WorkerThread *worker		= std::move (freeWorkers.front());
		pendingTasks.pop ();
		freeWorkers.pop_front ();
		worker->assignTask (newTask);
	}


	void DynamicThreadPool::createFreeWorker () {
		std::function<void (WorkerThread*)> afterCompFun	= [this] (WorkerThread *wt) {
			{
				std::unique_lock<std::mutex> lock (managerMutex);
				freeWorkers.push_back (wt);
			}

			workersCondition.notify_one ();
		};

		WorkerThread *wt	= new WorkerThread (afterCompFun);
		freeWorkers.emplace_front (wt);
		activeWorkersCount++;
	}


	void DynamicThreadPool::destroyFirstFreeWorker () {
		WorkerThread *worker	= std::move (freeWorkers.front());
		freeWorkers.pop_front ();
		delete (worker);
		activeWorkersCount--;
	}


	void DynamicThreadPool::resize () {
		// Trying to decrese workers
		while (freeWorkers.size()>0 && activeWorkersCount>upperLimit) {
			destroyFirstFreeWorker ();
		}
		// Trying to increase workers
		while (activeWorkersCount<lowerLimit) {
			createFreeWorker ();
		}
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
		if ((int) uLimit < lowerLimit) {
			std::string strLL	= std::to_string (lowerLimit);
			std::string strIUL	= std::to_string (uLimit);
			throw std::runtime_error ("Input value "+ strIUL +" is lower than current lower limit "+ strLL);
		}

		std::unique_lock<std::mutex> lock (managerMutex);
		upperLimit	= uLimit;
		resize ();
	}


	void DynamicThreadPool::unsetUpperLimit () {
		upperLimit	= -1;
	}




	void DynamicThreadPool::setLowerLimit (size_t lLimit) {
		if ((int) lLimit > upperLimit && upperLimit != -1) {
			std::string strUL	= std::to_string (upperLimit);
			std::string strILL	= std::to_string (lLimit);
			throw std::runtime_error ("Input value "+ strILL +" is greater than current upper limit "+ strUL);
		}

		std::unique_lock<std::mutex> lock (managerMutex);
		lowerLimit	= lLimit;
		resize ();
	}




	void DynamicThreadPool::unsetLowerLimit () {
		lowerLimit	= 0;
	}

} // namespace dynamicThreadPool


#endif // DYNAMIC_THREAD_POOL_H