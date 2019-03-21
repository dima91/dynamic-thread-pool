/**
 * \file test0.cpp
 * \brief Source file for Test0 executable
 * \author Luca Di Mauro
 */


#include <dynamicThreadPool.h>
#include <concurrentQueue.h>

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

	t01 ();

	//t02 ();

	cout << "\n\nBye bye!\n";

	return 0;
}




void t00 () {
	ConcurrentQueue<int> cQueue;
	int i	= 10;
	cQueue.push (i);
	i		= 100;
	cQueue.push (i);
	i		= 1000;
	cQueue.push (i);

	int j	= 0;
	cQueue.pop (j);
	cout << "First  : " << j << endl;
	cQueue.pop (j);
	cout << "Second : " << j << endl;
	cQueue.pop (j);
	cout << "Third  : " << j << endl;
}




void t01 () {

	std::shared_ptr<DynamicThreadPool> poolPtr	= std::make_shared<DynamicThreadPool> (4);
	
	/*DynamicThreadPool pool (5);
	DynamicThreadPool *poolPtr = &pool;//*/
	//cout << "use_count  " << pool.use_count () << "\n";

	auto handle	= std::async (std::launch::async, [poolPtr] {
		//cout << "Async use_count  " << pool.use_count () << "\n";
		//cout << "Inside async..\n";
		std::this_thread::sleep_for (std::chrono::milliseconds (10000));
		cout << "Stopping pool!\n";
		poolPtr->stop();

		return 8;
	});

	for (int i=0; i<20; i++) {
		poolPtr->submit ([i] () {
			cout<< "Task  " << i << "  done!\n";
		});

		std::this_thread::sleep_for (std::chrono::milliseconds (100));
	}

	poolPtr->join ();
}




void t02 () {
	std::shared_ptr<DynamicThreadPool> pool	= std::make_shared<DynamicThreadPool> (3);

	for (int i=0; i<20; i++) {
		pool->submit ([i] () {
			cout<< "Inside task " << i << endl;
			std::this_thread::sleep_for (std::chrono::milliseconds (1000));
		});
	}

	pool->join ();
}