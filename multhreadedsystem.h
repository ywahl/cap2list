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
	void run();
};



class MultiThreadedSystem : public System {
	pthread_t masterThreadTid;
	static std::shared_ptr<spdlog::logger> log;
	SingleThreadSystem **singleThreadSystems;
	int nThreads;
public:
	MultiThreadedSystem(int nThreads, int nMsg);
  	inline static std::shared_ptr<spdlog::logger> getLogger() { return log;}
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
