class MultiThreadedSystem;


class SingleThreadSystem : public BasicSystem {
	MultiThreadedSystem *parent;
	int idx;
	EpollTask pollTask;
	ProxyTask proxyTask;
	Thread scheduleThread;
	std::thread thr;
public:
    SingleThreadSystem(MultiThreadedSystem *ms);
	SingleThreadSystem(MultiThreadedSystem *ms, int id);
	int getIdx() { return idx;}
	void run();
};



class MultiThreadedSystem : public System {
	pthread_t masterThreadTid;
	static std::shared_ptr<spdlog::logger> log;
	SingleThreadSystem **singleThreadSystems;
	int nThreads;
public:
	MultiThreadedSystem(int nThreads, int nMsg);
  	static std::shared_ptr<spdlog::logger> getLogger();
	void init();
	void postMsg(Message *d);
	void registerTask(Task *);
	~MultiThreadedSystem();
	Message *getMessage();
	Message *getMessage(int nThread);
	void releaseMessage(Message *msg);
	Task *getTask(const char *name);
        void startLoop();
};
