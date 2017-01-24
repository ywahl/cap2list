/*
 * base.h
 *
 *  Created on: Nov 30, 2014
 *      Author: awahl
 */

#ifndef BASE_H_
#define BASE_H_

#include <vector>
#include <string.h>
#include <stdexcept>

class Object {
};

inline std::size_t
  __stl_hash_string(const char* __s)
  {
    unsigned long __h = 0;
    for ( ; *__s; ++__s)
      __h = 5 * __h + *__s;
    return size_t(__h);
  }

struct hash_const_char
{
	std::size_t
      operator()(const char* __s) const
      { return __stl_hash_string(__s); }
};

enum MsgType {
	cfgMsg,
	initMsg,
	stopMsg,
	resetMsg,
	subscribeMsg,
	unSubscribeMsg,
	writeMsg,
	readMsg,
	connectedMsg,
	disconnectedMsg,
	getStatusMsg,
	pollMsg,
	timeOutMsg,
	pingMsg,
	lastMsg
};

//TODO don't use STL, use static ring buffers
#define BUFFER_BOOL_CANARY_SIZE 4

template<typename T>
class BufferPool : public Object {
private:
	char *memChunk;
	std::vector<T *> bufferDescriptors;
	char canary[BUFFER_BOOL_CANARY_SIZE];
public:
	BufferPool(int nBuffer, int szBuffer, const char *cnry);
	T *getBuffer();
	void releaseBuffer(T *);
	int getLength() { return bufferDescriptors.size(); }
};

template<class T>
BufferPool<T>::BufferPool(int nBuffer, int szBuffer, const char *cnry)
{
	memcpy(canary, cnry, BUFFER_BOOL_CANARY_SIZE);

	memChunk = new char[nBuffer * (szBuffer + BUFFER_BOOL_CANARY_SIZE)];

	for (int i = 0; i < nBuffer; i++) {
		char *ptr = memChunk + i * (szBuffer + BUFFER_BOOL_CANARY_SIZE);
		memcpy(ptr, canary, BUFFER_BOOL_CANARY_SIZE);
		bufferDescriptors.push_back(reinterpret_cast<T *>(ptr + BUFFER_BOOL_CANARY_SIZE));
	}
}

template<class T>
T *BufferPool<T>::getBuffer()
{
	T *buffer;
	if (bufferDescriptors.size() == 0) {
		throw std::runtime_error("BufferPool: Out of buffers");
		return NULL;
	}
	buffer = bufferDescriptors.back();
	bufferDescriptors.pop_back();

	return buffer;
}

template<class T>
void BufferPool<T>::releaseBuffer(T *buf)
{
	if (buf == NULL)
		throw std::runtime_error("BufferPool: NULL buffer released");
	char *ptr = reinterpret_cast<char *>(buf) - BUFFER_BOOL_CANARY_SIZE;
	if (memcmp(ptr, canary, BUFFER_BOOL_CANARY_SIZE ))
		throw std::runtime_error("BufferPool: releaseBuffer: corrupted");
	bufferDescriptors.push_back(buf);
}

#define MSG_FLAG_IDLE 0x1

class Task;

struct Message {
	MsgType type;
	int subtype;
	Task *src;
	Task *dst;
	int flags;
	Message *nextMsg;

	void *data;
	char smallData[32];
};


#endif /* BASE_H_ */
