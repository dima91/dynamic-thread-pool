/**
 * \file dynamicityTest.cpp
 * \brief Source file to test thread pool dynamicity
 * \author Luca Di Mauro
 */


#include <dynamicThreadPool.h>

#include <iostream>
#include <unistd.h>

#define UNO


using namespace std;
using namespace dynamicThreadPool;


int main (int argc, char **argv) {

	cout << "Hello user!\n\n";

	DynamicThreadPool pool;
	pool.setUpperLimit (5);
	DynamicThreadPool *poolPtr	= &pool;


	#ifdef UNO
	cout << "INIT --> Pool size: " << poolPtr->workersCount () << endl;

	for (int i=0; i<30; i++) {
		pool.submit ([poolPtr, i] {
			cout << "Pool size: " << poolPtr->workersCount () << endl;
			std::this_thread::sleep_for (std::chrono::seconds (1));
			cout << "End of task  " << i << endl;
		});
		//std::this_thread::sleep_for (std::chrono::milliseconds (500));
	}

	//std::this_thread::sleep_for (std::chrono::seconds (5));
	//pool.setLowerLimit (2);
	this_thread::sleep_for (std::chrono::seconds (4));
	pool.setUpperLimit (2);
	this_thread::sleep_for (std::chrono::seconds (2));

	pool.submit ([poolPtr] {
		cout << "Pool size: " << poolPtr->workersCount () << endl;
		std::this_thread::sleep_for (std::chrono::seconds (1));
		cout << "End of task!\n";
	});

	std::async (std::launch::async, [poolPtr] {
		std::this_thread::sleep_for (std::chrono::seconds (10));
		cout << "Stopping\n";
		poolPtr->stop ();
	});

	poolPtr->join ();

	#endif




	#ifdef TRE
	pool.setLowerLimit (2);
	pool.setUpperLimit (10);

	for (int h=0; h<30; h++) {
		pool.submit ([poolPtr] {
			cout << "Pool size: " << poolPtr->size () << endl;
			std::this_thread::sleep_for (std::chrono::seconds (1));
			cout << "End of task!\n";
		});
	}

	std::thread t1	= std::thread ([poolPtr] {
		std::this_thread::sleep_for (std::chrono::seconds (2));
		cout << "\n\n\nDecreasing\n";
		poolPtr->setUpperLimit (4);
		poolPtr->setLowerLimit (4);
	});

	std::thread t2	= std::thread ([poolPtr] {
		std::this_thread::sleep_for (std::chrono::seconds (10));
		cout << "\n\n\nStopping\n";
		poolPtr->stop ();
	});

	pool.join ();
	#endif


	//this_thread::sleep_for (std::chrono::milliseconds (1));

	/*pool.stop ();

	pool.join ();*/
	


	cout << "\n\nBye bye!\n";

	return 0;
}