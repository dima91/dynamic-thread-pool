/**
 * \file concurrentQueue.h
 * \brie Header file for ConcurrentQueue class
 * \author Luca Di Mauro
 */


#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>


namespace dynamicThreadPool {

	class ClosedQueueException : public std::runtime_error {
	public:
		ClosedQueueException (const char *msg) : runtime_error(msg) { /*Constructor do nothing*/ };
		virtual ~ClosedQueueException () throw() { /*Destructor do nothing*/ };
		virtual const char *what() const throw() { return runtime_error::what(); };
	};





	template <typename T>
	class ConcurrentQueue {
	private:
		std::queue<T>			tQueue;
  		std::mutex				queueMutex;
		std::condition_variable	queueCV;

		std::atomic<bool> closeMe;


	public:
		ConcurrentQueue ();
		~ConcurrentQueue ();

		void push		(T& element);
		void pop		(T& element);
		
		bool active		();
		bool isEmpty	();
		void close		();
	};
	

	
	
	/* ================================================== */
	/* ==============   Implementation   ================ */
	
	template <typename T>
	ConcurrentQueue<T>::ConcurrentQueue () : closeMe (false) {
		// Do nothing
	}
	

	template <typename T>
	ConcurrentQueue<T>::~ConcurrentQueue () {
		if (!closeMe)
			close ();
	}


	template <typename T>
	void ConcurrentQueue<T>::push (T& element) {
		{
			std::unique_lock<std::mutex> lock (queueMutex);
			tQueue.emplace (element);
		}

		queueCV.notify_all ();
	}


	template <typename T>
	void ConcurrentQueue<T>::pop (T& element) {
		{
			std::unique_lock<std::mutex> lock (queueMutex);
			
			while (tQueue.empty () && !closeMe)
				queueCV.wait (lock, [&] {return !tQueue.empty() || closeMe;});
			
			if (closeMe)
				throw ClosedQueueException ("Closed ConcurrentQueue");

			element = std::move (tQueue.front());
			tQueue.pop();
		}
		queueCV.notify_all ();
	}


	template <typename T>
	bool ConcurrentQueue<T>::isEmpty () {
		std::lock_guard<std::mutex> lock (queueMutex);
		return tQueue.empty();
	}


	template <typename T>
	bool ConcurrentQueue<T>::active () {
		return closeMe == false;
	}


	template <typename T>
	void ConcurrentQueue<T>::close () {
		closeMe	= true;
		queueCV.notify_all ();
	}

} // namespace dynamicThreadPool