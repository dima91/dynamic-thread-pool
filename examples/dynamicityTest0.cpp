/**
 * \file dynamicityTest0.cpp
 * \brief Source file for DynamicityTest0 executable
 * \author Luca Di Mauro
 */


#include <dynamicThreadPool.h>

#include <iostream>
#include <random>
#include <unistd.h>


using namespace std;
using namespace dynamicThreadPool;




int main (int argc, char **argv) {
	cout << "===============\nDynamicityTest0\n\n";
	
	// Creating a pool with no upper limit
	DynamicThreadPool pool;
	DynamicThreadPool *poolPtr	= &pool;

	pool.setLowerLimit (2);
	pool.setUpperLimit (10);

	for (int h=0; h<60; h++) {
		pool.submit ([h] {
			std::this_thread::sleep_for (std::chrono::seconds (1));
			cout << "End of task  " << h << endl;
		});
	}

	std::thread t1	= std::thread ([poolPtr] {
		std::this_thread::sleep_for (std::chrono::seconds (2));
		cout << "\n\n\nChanging limits..\n";
		poolPtr->setUpperLimit (4);
		poolPtr->setLowerLimit (4);
	});

	std::thread t2	= std::thread ([poolPtr] {
		std::this_thread::sleep_for (std::chrono::seconds (10));
		cout << "\n\n\nStopping\n";
		poolPtr->stop ();
	});

	pool.join ();
	t1.join ();
	t2.join ();

	cout << "\n\n==========\nTest done!\n";

	return 0;
}