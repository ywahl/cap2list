/*
 * config.cpp
 *
 *  Created on: Nov 26, 2014
 *      Author: awahl
 */
#include <string.h>
#include "config.h"


void Configuration::set(const char *key, const char *value)
{
	config[key] = value;
}

void Configuration::unset(const char *key)
{
	ConfigMap::iterator iter;

	iter = config.find(key);

	if (iter != config.end())
		config.erase(iter);
}

const char *Configuration::get(const char *key)
{
	ConfigMap::iterator iter;

	iter = config.find(key);

	if (iter != config.end())
		return iter->second;

	return NULL;
}

bool Configuration::get(const char *key, bool keyfound)
{
	ConfigMap::iterator iter;

	iter = config.find(key);

	if (iter == config.end())
		return false;

	if (keyfound)
		return true;

	const char *answer = iter->second;

	if (strncmp("true", answer, 4) == 0)
		return true;

	return false;
}

int Configuration::get(const char *key, int def)
{
	const char *value = get(key);
	if (value == NULL)
		return def;
	return atoi(value);
}

