/**
 * \file dynamicThreadPool.h
 * \brie Header file for DynamicThreadPool class
 * \author Luca Di Mauro
 */


#include <concurrentQueue.h>

#include <iostream>
#include <future>
#include <vector>
#include <functional>


namespace dynamicThreadPool {

	using DefaultCallback	= std::function<void()>;


	class DynamicThreadPool {
	private:
		int upperLimit;
		int lowerLimit;

		std::vector<std::thread> workers;
		ConcurrentQueue<DefaultCallback> taskQueue;

		bool haltMe;	// TODO Make it atomic

		DefaultCallback workerCallback	= [&] {
			DefaultCallback task;

			while (!haltMe) {
				try {
					taskQueue.pop (task);
					task ();
				} catch (EmptyQueueException &err) {
					// Wiating before retry
					break ;
				}
			}
		};


	public:
		DynamicThreadPool	(size_t initCapacity=0);
		~DynamicThreadPool	();

		template<class F, class... Args>
		auto submit (F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

		bool active	();
		void stop	();
		void join	();

		void setUpperLimit		(size_t limit);
		void unsetUpperLimit	();
		void setLowerLimit		(size_t limit);
		void unsetLowerLimit	();
	};
	



	/* ================================================== */
	/* ==============   Implementation   ================ */

	DynamicThreadPool::DynamicThreadPool (size_t initCapacity) : haltMe(false) {
		workers.reserve (initCapacity);
	    for (size_t i=0; i<initCapacity; i++) {
	        workers.emplace_back (workerCallback);
		}
	}
	

	DynamicThreadPool::~DynamicThreadPool () {
		stop	();
		join	();
	}


	template<typename F, typename...Args>
	auto DynamicThreadPool::submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
		using ReturnType	= decltype(f(args...));

		auto fun					= std::bind (std::forward<F>(f), std::forward<Args>(args)...);
		auto taskPtr				= std::make_shared<std::packaged_task<ReturnType()>> (fun);
		DefaultCallback wrapperFun	= [taskPtr] {(*taskPtr)();};
		
		taskQueue.push (wrapperFun);
		
		return taskPtr->get_future();
	}


	bool DynamicThreadPool::active () {
		return haltMe == false;
	}


	void DynamicThreadPool::stop () {
		haltMe	= true;
		taskQueue.close ();
	}


	void DynamicThreadPool::join () {
		for (auto &worker: workers)
			if (worker.joinable())
        		worker.join();
	}


	void DynamicThreadPool::setUpperLimit (size_t limit) {
		// TODO
	}


	void DynamicThreadPool::unsetUpperLimit () {
		// TODO
	}


	void DynamicThreadPool::setLowerLimit (size_t limit) {
		// TODO
	}


	void DynamicThreadPool::unsetLowerLimit () {
		// TODO
	}

} // namespace dynamicThreadPool