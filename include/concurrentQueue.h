/**
 * \file concurrentQueue.h
 * \brie Header file for ConcurrentQueue class
 * \author Luca Di Mauro
 */


#include <queue>
#include <mutex>
#include <condition_variable>


namespace dynamicThreadPool {

	class EmptyQueueException : public std::runtime_error {
	public:
		EmptyQueueException (const char *msg) : runtime_error(msg) { /*Constructor do nothing*/ };
		virtual ~EmptyQueueException () throw() { /*Destructor do nothing*/ };
		virtual const char *what() const throw() { return runtime_error::what(); };
	};





	template <typename T>
	class ConcurrentQueue {
	private:
		std::queue<T>			tQueue;
  		std::mutex				queueMutex;
		std::condition_variable	queueCV;

		bool closeMe;


	public:
		ConcurrentQueue ();
		~ConcurrentQueue ();

		void push		(T& element);
		void pop		(T& element, bool blockMe=true);
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
		// Do nothing
	}


	template <typename T>
	void ConcurrentQueue<T>::push (T& element) {
		{
			std::unique_lock<std::mutex> lock (queueMutex);
			tQueue.emplace (element);
		}

		queueCV.notify_one ();
	}


	template <typename T>
	void ConcurrentQueue<T>::pop (T& element, bool blockMe) {
		{
			std::unique_lock<std::mutex> lock (queueMutex);
			if (blockMe) {
				queueCV.wait (lock, [&] {return !tQueue.empty() || closeMe;});
			}
			else if (tQueue.empty()) {
				throw EmptyQueueException ("The queue is empty");
			}

			if (!tQueue.empty() && !closeMe) {
				element = std::move (tQueue.front());
				tQueue.pop();
			}
			else {
				throw EmptyQueueException ("The queue is empty");
			}
		}
		queueCV.notify_one ();
	}


	template <typename T>
	bool ConcurrentQueue<T>::isEmpty () {
		std::lock_guard<std::mutex> lock (queueMutex);
		return tQueue.empty();
	}


	template <typename T>
	void ConcurrentQueue<T>::close () {
		{
			std::lock_guard<std::mutex> lock (queueMutex);
			closeMe	= true;
		}
		queueCV.notify_all ();
	}

} // namespace dynamicThreadPool