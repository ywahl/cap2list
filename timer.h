/*
 * timer.h
 *
 *  Created on: Oct 26, 2014
 *      Author: awahl
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <sys/timerfd.h>
#include "poll.h"
#include "queue.h"

enum TimerType {
	tmr_oneshot,
	tmr_periodic
};

struct TimerData {
	Task *task;
	int timeout;
	TimerType type;
};

class TimerTask : public Task {
	int clock_fd;
	u_int32_t ticks;
	std::vector<TimerData *> notifyTaskList;
	BufferPool<TimerData> pool;
public:
	TimerTask(System *s) : Task(s, "timer"), clock_fd(0), ticks(0), pool(16, sizeof(TimerData), "TIME") {}
	void execute(Message *);
	void init(Task *eventTask);
	void notifyTasks();
};



#endif /* TIMER_H_ */
