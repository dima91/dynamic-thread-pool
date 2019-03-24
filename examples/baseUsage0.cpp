/**
 * \file baseUsage0.cpp
 * \brief Source file for BaseUsage1 executable
 * \author Luca Di Mauro
 */


#include <dynamicThreadPool.h>

#include <iostream>
#include <random>
#include <unistd.h>


using namespace std;
using namespace dynamicThreadPool;




int main (int argn, char **argv) {
	cout << "=========\nBaseUsage0\n\n";

	DynamicThreadPool pool;		// Initialized with 0 workers
	pool.setUpperLimit (5);

	// Submitting 20 tasks, each of them waits a variable amount of time between 0.5 and 1.5 seconds
	random_device					rd;
    mt19937							mt(rd());
    uniform_int_distribution<int>	dist(0, 1000);

	for (int i=0; i<20; i++) {
		int msDelay	= dist(mt) + 500;
		
		pool.submit ([] (int idx, int delay) {
			std::this_thread::sleep_for (std::chrono::milliseconds (delay));
			cout<< "Task  " << idx << "  done!\n";
		}, i, msDelay);
	}

	// Activating an async task which will stop the thread pool
	DynamicThreadPool *poolPtr	= &pool;
	async (launch::async, [poolPtr] {
		std::this_thread::sleep_for (std::chrono::seconds (8));
		poolPtr->stop ();
	});

	// Waiting for pool stop
	pool.join ();

	cout << "\n\n==========\nTest done!\n";

	return 0;
}