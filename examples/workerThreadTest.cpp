/**
 * \file workerThreadTest.cpp
 * \brief Source file to test WorkerThread class
 * \author Luca Di Mauro
 */


#include <dynamicThreadPool.h>

#include <iostream>
#include <unistd.h>


using namespace std;
using namespace dynamicThreadPool;


int main (int argc, char **argv) {

	cout << "\n===========\nHello user!\n\n";

	WorkerThread wt0 ([] (WorkerThread *thisWT) {
		cout << "Put me at the end of the list!\n";
	});
	WorkerThread *wt0Ptr	= &wt0;

	cout << "Computing?  " << wt0.isComputing() << endl;
	cout << "Status?     " << wt0.getStatus() << endl;

	std::thread par1 ([wt0Ptr] {
		std::this_thread::sleep_for (std::chrono::milliseconds (2000));
		wt0Ptr->assignTask ([] {
			cout << "Inside this task!";
		});
		std::this_thread::sleep_for (std::chrono::milliseconds (2000));
		wt0Ptr->stop ();
	});

	cout << "Joining..\n";
	wt0.join ();
	cout << "Joined!\n";
	par1.join ();



	cout << "\n\n/* ================================================== */\n";
	cout << "/* ================================================== */\n\n";




	WorkerThread *wt1	= new WorkerThread ([] (WorkerThread *wt){
		cout << "Do nothing\n";
	});

	auto task	= [] (string msg, int msToSleep) {
		std::this_thread::sleep_for (std::chrono::milliseconds (msToSleep));
		cout << msg << endl;
	};


	std::thread par2 ([wt1, task] {
		auto taskPtr	= task;
		wt1->assignTask ([taskPtr] {taskPtr ("First task", 1000);});
		std::this_thread::sleep_for (std::chrono::milliseconds (500));
		cout << "Computing?  " << wt1->isComputing () << endl;
		wt1->assignTask ([wt1, taskPtr] {taskPtr ("Second task", 100); wt1->stop ();});

		wt1->join();
	});

	par2.join ();



	cout << "\n\nBye bye!\n========\n";
}