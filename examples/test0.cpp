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


int main (int argc, char **argv) {

	cout << "Hello user!\n\n";
	
	t00 ();

	t01 ();

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

	DynamicThreadPool pool (10);
	DynamicThreadPool *poolPtr	= &pool;

	auto res1	= pool.submit ([poolPtr] {
		std::this_thread::sleep_for (std::chrono::seconds (11));
		poolPtr->stop ();
	});

	for (int i=0; i<20; i++) {
		pool.submit ([i] () {
			cout<< "Inside task " << i << endl;
			std::this_thread::sleep_for (std::chrono::milliseconds (250));
		});
	}

	pool.join ();
}