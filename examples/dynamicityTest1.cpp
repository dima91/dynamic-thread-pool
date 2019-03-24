/**
 * \file dynamicityTest1.cpp
 * \brief Source file for DynamicityTest1 executable
 * \author Luca Di Mauro
 */


#include <dynamicThreadPool.h>

#include <iostream>
#include <random>
#include <unistd.h>


using namespace std;
using namespace dynamicThreadPool;




int main (int argc, char **argv) {
	cout << "===============\nDynamicityTest1\n\n";
	
	// Creating a pool with no upper limit and no workers
	DynamicThreadPool pool;
	DynamicThreadPool *poolPtr	= &pool;

	cout << "INIT --> Pool size: " << pool.workersCount () << endl;

	for (int i=0; i<30; i++) {
		pool.submit ([poolPtr, i] {
			cout << "Pool size: " << poolPtr->workersCount () << endl;
			std::this_thread::sleep_for (std::chrono::seconds (1));
			cout << "End of task  " << i << endl;
		});
		std::this_thread::sleep_for (std::chrono::milliseconds (100));
	}

	this_thread::sleep_for (std::chrono::seconds (4));
	pool.setUpperLimit (2);
	this_thread::sleep_for (std::chrono::seconds (2));

	pool.submit ([poolPtr] {
		cout << "Pool size: " << poolPtr->workersCount () << endl;
		std::this_thread::sleep_for (std::chrono::seconds (1));
		cout << "End of task!\n";
	});

	std::async (std::launch::async, [poolPtr] {
		std::this_thread::sleep_for (std::chrono::seconds (5));
		cout << "Stopping\n";
		poolPtr->stop ();
	});

	pool.join ();

	cout << "\n\n==========\nTest done!\n";

	return 0;
}