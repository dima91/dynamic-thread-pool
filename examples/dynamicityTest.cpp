/**
 * \file dynamicityTest.cpp
 * \brief Source file to test thread pool dynamicity
 * \author Luca Di Mauro
 */


#include <dynamicThreadPool.h>

#include <iostream>
#include <unistd.h>


using namespace std;
using namespace dynamicThreadPool;


int main (int argc, char **argv) {

	cout << "Hello user!\n\n";

	DynamicThreadPool pool;
	DynamicThreadPool *poolPtr	= &pool;

	/*for (int i=0; i<10; i++) {
		pool.submit ([poolPtr] {
			cout << "Pool size: " << poolPtr->size () << endl;
			std::this_thread::sleep_for (std::chrono::seconds (1));
			cout << "End of task!\n";
		});
	}

	pool.setLowerLimit (5);
	this_thread::sleep_for (std::chrono::seconds (4));
	pool.setUpperLimit (3);
	this_thread::sleep_for (std::chrono::seconds (2));*/

	/*pool.submit ([poolPtr] {
		cout << "Pool size: " << poolPtr->size () << endl;
		std::this_thread::sleep_for (std::chrono::seconds (1));
		cout << "End of task!\n";
	});*/

	pool.setLowerLimit (5);
	this_thread::sleep_for (std::chrono::milliseconds (1));
	pool.setLowerLimit (1);

	pool.submit ([poolPtr] {
		cout << "Pool size: " << poolPtr->size () << endl;
		std::this_thread::sleep_for (std::chrono::seconds (1));
		cout << "End of task!\n";
	});

	this_thread::sleep_for (std::chrono::milliseconds (1));

	pool.stop ();

	pool.join ();
	


	cout << "\n\nBye bye!\n";

	return 0;
}