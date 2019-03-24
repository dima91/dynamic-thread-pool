/**
 * \file baseUsage1.cpp
 * \brief Source file for BaseUsage2 executable
 * \author Luca Di Mauro
 */


#include <dynamicThreadPool.h>

#include <iostream>
#include <random>
#include <unistd.h>


using namespace std;
using namespace dynamicThreadPool;




int main (int argc, char **argv) {
	cout << "=========\nBaseUsage1\n\n";
	
	// Creating a pool with no upper limit: each task will be done in a separate worker
	std::shared_ptr<DynamicThreadPool> pool	= std::make_shared<DynamicThreadPool> ();

	// Submitting 20 tasks, each of them waits a variable amount of time between 0.5 and 1.5 seconds
	random_device					rd;
    mt19937							mt(rd());
    uniform_int_distribution<int>	dist(0, 1000);

	for (int i=0; i<20; i++) {
		int msDelay	= dist(mt) + 500;
		
		pool->submit ([] (int idx, int delay) {
			std::this_thread::sleep_for (std::chrono::milliseconds (delay));
			cout<< "Task  " << idx << "  done!\n";
		}, i, msDelay);
	}


	async (std::launch::async, [pool] {
		std::this_thread::sleep_for (std::chrono::milliseconds (3000));
		pool->stop ();
	});

	cout << "\n\n==========\nTest done!\n";

	return 0;
}