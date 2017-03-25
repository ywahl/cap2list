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
	SingleThreadSystem(MultiThreadedSystem *ms, int id);
	void run();
};






class MultiThreadedSystem : public System {
	pthread_t masterThreadTid;
	static std::shared_ptr<spdlog::logger> log;
	SingleThreadSystem **singleThreadSystems;
	int nThreads;
public:
	MultiThreadedSystem(int nThreads, int nMsg);
	void init();
	void postMsg(Message *d);
	void registerTask(Task *);
	~MultiThreadedSystem();
	Message *getMessage();
	Message *getMessage(int nThread);
	void releaseMessage(Message *msg);
	Task *getTask(const char *name);
	inline static std::shared_ptr<spdlog::logger> getLogger() { return log;}
};

void SingleThreadSystem::run()
{
	int port = 10000 + idx;
	std::string portStr = std::to_string(port);
	MultiThreadedSystem::getLogger()->info("SingleThreadSystem thread created with idx={}", idx);
	proxyTask.config.set("server", "true");
	proxyTask.config.set("ipaddress", "127.0.0.1");
	proxyTask.config.set("port", portStr.c_str());
	Message *msg = getMessage();
	msg->type = initMsg;
	msg->dst = &proxyTask;
	postMsg(msg);
	getScheduler()->setIdleTask(&pollTask);
	scheduleThread.loop();
}


std::shared_ptr<spdlog::logger> MultiThreadedSystem::log = spdlog::stdout_color_mt("console");

SingleThreadSystem::SingleThreadSystem(MultiThreadedSystem *ms, int id) : BasicSystem(1000), parent(ms), idx(id),
		pollTask(this, "epollTask", 100, 100), proxyTask(idx, this, &pollTask),
		scheduleThread(this->getScheduler()), thr([=] {run();})
{
	MultiThreadedSystem::getLogger()->info("SingleThreadSystem created with idx={}", id);
}






MultiThreadedSystem::MultiThreadedSystem(int nThreads, int numOfMsg) : nThreads(nThreads)
{
  masterThreadTid = tid;
  singleThreadSystems = new SingleThreadSystem*[nThreads];
  for (int i = 0; i <= nThreads; i++) {
	  singleThreadSystems[i] = new SingleThreadSystem(this, i);
  }
}



void MultiThreadedSystem::init()
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
	if (tid != masterThreadTid) {
		getLogger()->error("getMessage of multithread system can only be called on the main thread {}", masterThreadTid);
		return NULL;
	}
	return singleThreadSystems[0]->getMessage();
}


void MultiThreadedSystem::releaseMessage(Message* msg) {

}


Task* MultiThreadedSystem::getTask(const char* name) {
	return NULL;
}



class KTask : public Task {
public:
	KTask(const char *name): Task(name) {}
	void execute(Message *msg) {
		MultiThreadedSystem::getLogger()->info("KTask rcv msg {}", "yaron");
	}
};



int main(int argc, char *argv[])
{
	MultiThreadedSystem sys(2, 200);
	MultiThreadedSystem::getLogger()->info("Application starting master thread {}", tid);
	Message *msg = sys.getMessage();
	KTask ktask("bob");
	msg->type = initMsg;
	msg->dst = &ktask;

	sys.postMsg(msg);
	while(true)
		sleep(1);
	return 0;
}
