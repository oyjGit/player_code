#ifndef __JSONSUBTITUTE_H__
#define __JSONSUBTITUTE_H__
#include <string.h>
#include "json/json.h"

class JsonSubtitute {
public:
	JsonSubtitute() {
		_parsed = false;
	}
	bool ParseJson(const char* json) {
		if (_parsed) {
			return true;
		}
		_parsed = _jreader.parse(json, _jroot);
		return _parsed;
	}
	inline Json::Value& RootObject() {
		return _jroot;
	}
	inline Json::Value& GetObject(Json::Value& jobject, const char* key) {
		return jobject[key];
	}
	bool Subtitute(Json::Value& jobject, const char* key, const char* value) {
		if (!_parsed) {
			return false;
		}
		Json::Value& jvalue = _jroot[key];
		if (jvalue.isNull()) {
			return false;
		}
		jvalue = value;
		return true;
	}
	inline bool Subtitute(const char* key, const char* value) {
		return this->Subtitute(this->RootObject(), key, value);
	}
	inline char* JsonDup() const {
		if (_parsed) {
			return strdup(_jroot.toStyledString().c_str());
		}
		return NULL;
	}
private:
	bool _parsed;
	Json::Reader _jreader;
	Json::Value _jroot;
};

#endif //__JSONSUBTITUTE_H__
