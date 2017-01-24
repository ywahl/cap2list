/*
 * config.h
 *
 *  Created on: Nov 25, 2014
 *      Author: awahl
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <unordered_map>
#include "base.h"

//
//Should we use a templated approach?
//
class Variable : public Object {
public:
	virtual ~Variable() {}
	virtual const char *getValue() = 0;
	virtual void setValue(const char *valueStr) = 0;
};

typedef const char * (*GetValueFunction)(void *);
typedef const void(*SetValueFunction)(void *, const char *);

class FuncVariable : public Variable {
	void *context;
	GetValueFunction getF;
	SetValueFunction setF;
public:
	FuncVariable(void *ctx, GetValueFunction gF, SetValueFunction sF) : context(ctx), getF(gF), setF(sF) {}
	const char *getValue() {
		if (getF != NULL)
			return getF(context);
		return "";
	}
	void setValue(const char *valueStr) {
		if (setF != NULL)
			setF(context, valueStr);
	}
};

class BoolVariable : public Variable {
	bool *ptrValue;
public:
	BoolVariable(bool *boolPtr) : ptrValue(boolPtr) {}

	const char *getValue() {
		if (*ptrValue == true) return "true";
		return "false";
	}

	void setValue(const char *valueStr) {
		if (strncmp("true", valueStr, sizeof("true")) == 0) {
			*ptrValue = true;
			return;
		}
		*ptrValue = false;
	}
};

class IntVariable : public Variable {
	int *ptrValue;
	char printBuf[12];
public:
	IntVariable(int *intPtr) : ptrValue(intPtr) {}

	const char *getValue() {
		sprintf(printBuf, "%d", *ptrValue);
		return printBuf;
	}

	void setValue(const char *valueStr) {
		if (strncmp("true", valueStr, sizeof("true")) == 0) {
			*ptrValue = true;
			return;
		}
		*ptrValue = false;
	}
};

struct compare_const_char
{
	inline bool
      operator()(const char* str1, const char* str2) const
      { return strcmp(str1, str2) ? false : true; }
};


using ConfigMap =  std::unordered_map<const char*, const char *, hash_const_char, compare_const_char>;

class Configuration : public Object {

protected:
	ConfigMap config;
public:
	void set(const char *key, const char *value);
	void unset(const char *key);
	const char *get(const char *key);
	bool get(const char *key, bool keyfound);
	int get(const char *key, int def);
};

#endif /* CONFIG_H_ */
