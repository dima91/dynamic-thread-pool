/**
 * \file cqtest.cpp
 * \brie Source file for CqTest executable
 * \author Luca Di Mauro
 */


#include <concurrentQueue.h>

#include <iostream>
#include <thread>


using namespace std;
using namespace dynamicThreadPool;



int main (int argc, char **argv) {
	shared_ptr<ConcurrentQueue<int>> cq	= make_shared<ConcurrentQueue<int>> ();

	
	thread t1 ([cq] {
		int i	= 0;
		while (i<100) {
			cq->push (i);
			i++;
			this_thread::sleep_for (chrono::milliseconds (10));
		}
		cout << "-----  EXITING  -----\n\n";
	});

	this_thread::sleep_for (chrono::milliseconds (100));
	vector <thread> ts;

	for (int j=0; j<5; j++) {
		ts.emplace_back (([cq, j] () {
			int tmp;
			while (true) {
				try {
					cq->pop (tmp);
					cout << "Thread  " << j << "  poped  " << tmp << endl;
					this_thread::sleep_for (chrono::milliseconds (500));
				} catch (ClosedQueueException &e) {
					cout << "Catched the exception: closed queue" << endl;
					break ;
				}
			}
		}));
	}

	t1.join ();
	for (auto& t : ts)
		if (t.joinable())
			t.join();

	return 0;
}