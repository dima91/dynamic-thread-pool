/**
 * \file test0.cpp
 * \brief Source file for Test0 executable
 * \author Luca Di Mauro
 */


#include <dynamicThreadPool.h>

#include <iostream>
#include <unistd.h>


using namespace std;
using namespace dynamicThreadPool;

void t00 ();
void t01 ();
void t02 ();


int main (int argc, char **argv) {

	cout << "Hello user!\n\n";
	
	//t00 ();

	//t01 ();

	t02 ();

	cout << "\n\nBye bye!\n";

	return 0;
}




void t00 () {
	auto h1	= std::async (std::launch::async, [] {
		//cout << "Async use_count  " << pool.use_count () << "\n";
		//cout << "Inside async..\n";
		std::this_thread::sleep_for (std::chrono::milliseconds (4000));
		cout << "Wake up!\n";

		return 8;
	});


	auto h2	= std::async (std::launch::async, [] {
		while (1){
			cout << "Iteration..\n";
			std::this_thread::sleep_for (std::chrono::milliseconds (5000));
		}

		return 8;
	});	
}




void t01 () {

	std::shared_ptr<DynamicThreadPool> poolPtr	= std::make_shared<DynamicThreadPool> (2);

	auto handle	= std::async (std::launch::async, [poolPtr] {
		//cout << "Async use_count  " << pool.use_count () << "\n";
		//cout << "Inside async..\n";
		std::this_thread::sleep_for (std::chrono::milliseconds (4000));
		cout << "wc: " << poolPtr->workersCount() << "\ttc: " << poolPtr->tasksCount() << "\tfwc: " << poolPtr->freeWorkersCount() << endl;
		cout << "Stopping pool!\n";
		poolPtr->stop();

		return 8;
	});

	int i	= 0;
	while (1) {
		i++;
		try {
			poolPtr->submit ([i] () {
				cout<< "Task  " << i << "  done!\n";
				std::this_thread::sleep_for (std::chrono::milliseconds (300));
			});
		} catch (...) {
			break;
		}

		std::this_thread::sleep_for (std::chrono::milliseconds (100));
	}


	poolPtr->join ();

	std::cout << "Exited!\n";
}




void t02 () {
	std::shared_ptr<DynamicThreadPool> pool	= std::make_shared<DynamicThreadPool> (3);

	for (int i=0; i<20; i++) {
		pool->submit ([i] () {
			cout<< "Inside task " << i << endl;
			std::this_thread::sleep_for (std::chrono::milliseconds (1000));
		});
	}

	auto handle	= std::async (std::launch::async, [pool] {
		std::this_thread::sleep_for (std::chrono::milliseconds (5000));
		pool->stop ();
	});

	pool->join ();
}




