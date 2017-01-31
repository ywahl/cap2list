#include <iostream>
#include <memory>
#include <thread>
#include <pthread.h>
#include "spdlog/spdlog.h"
#include "queue.h"
#include "poll.h"
#include "proxytask.h"




thread_local  pthread_t tid = pthread_self();


class MultiThreadedSystem;


class SingleThreadSystem : public BasicSystem {
	MultiThreadedSystem *parent;
	int idx;
	EpollTask pollTask;
	ProxyTask proxyTask;
	Thread scheduleThread;
	std::thread thr;
public:
	SingleThreadSystem(MultiThreadedSystem *ms, int id) : thr(&SingleThreadSystem::run, this) {}
	void run();
};






class MultiThreadedSystem : public System {
	pthread_t masterThreadTid;
	static std::shared_ptr<spdlog::logger> log;
	SingleThreadSystem **singleThreadSystems;
public:
	MultiThreadedSystem(int nThreads, int nMsg);
	void init();
	void postMsg(Message *d);
	void registerTask(Task *);
	~MultiThreadedSystem();
	Message *getMessage();
	void releaseMessage(Message *msg);
	Task *getTask(const char *name);
	inline static std::shared_ptr<spdlog::logger> getLogger() { return log;}
};

void SingleThreadSystem::run()
{
	MultiThreadedSystem::getLogger()->info("SingleThreadSystem thread created with idx={}", idx);
	scheduleThread.loop();
}

std::shared_ptr<spdlog::logger> MultiThreadedSystem::log = spdlog::stdout_color_mt("console");

SingleThreadSystem::SingleThreadSystem(MultiThreadedSystem *ms, int id) : BasicSystem(1000), parent(ms), idx(id),
		pollTask(this, "pollTask", 100, 100), proxyTask(idx, this, &pollTask),
		scheduleThread(this->getScheduler())
{
	MultiThreadedSystem::getLogger()->info("SingleThreadSystem created with idx={}", id);
}






MultiThreadedSystem::MultiThreadedSystem(int nThreads, int numOfMsg)
{
  masterThreadTid = tid;
  singleThreadSystems = new SingleThreadSystem*[nThreads];
  for (int i = 0; i < nThreads; i++) {
	  singleThreadSystems[i] = new SingleThreadSystem(this, i);

  }



}



void init()
{

}


void MultiThreadedSystem::registerTask(Task *t)
{
}

void MultiThreadedSystem::postMsg(Message *msg)
{
}




MultiThreadedSystem::~MultiThreadedSystem() {
}


Message* MultiThreadedSystem::getMessage() {
	return NULL;
}


void MultiThreadedSystem::releaseMessage(Message* msg) {

}


Task* MultiThreadedSystem::getTask(const char* name) {
	return NULL;
}


int main(int argc, char *argv[])
{
	MultiThreadedSystem sys(2, 200);
	MultiThreadedSystem::getLogger()->info("Application starting master thread {}", tid);
	return 0;
}
