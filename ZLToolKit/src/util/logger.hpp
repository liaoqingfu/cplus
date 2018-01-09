/*
 * MIT License
 *
 * Copyright (c) 2016 xiongziliang <771730766@qq.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef UTIL_LOGGER_H_
#define UTIL_LOGGER_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <deque>
#include <map>
#include <ctime>
#include <string.h>
#include <cstdlib>
#include <thread>
#include <memory>
#include <mutex>
#include <time.h>
#include <condition_variable>
//#include <process.h>
#include "semaphore.hpp"


using namespace std;
using namespace ZL::Thread;

namespace ZL
{
namespace Util 
{
enum LogLevel
{
	LTrace  = 0, 
	LDebug, 
	LInfo, 
	LWarn, 
	LError, 
	LFatal,
};
static const char *LogLevelStr[] = 
{
	"trace",
	"debug",
	"info",
	"warn",
	"error",
	"fatal"
};

// 字符颜色可以参考 http://blog.csdn.net/panpan639944806/article/details/23930553
#define CLEAR_COLOR "\033[0m"
#define UNDERLINE	"\033[4m"

static const char *COLOR[6][2] = 
{ 
	{ "\033[44;37m", "\033[34m" }, 
	{ "\033[42;37m", "\033[32m" }, 
	{ "\033[46;37m", "\033[36m" }, 
	{ "\033[43;37m", "\033[33m" }, 
	{ "\033[45;37m", "\033[35m" }, 
	{ "\033[41;37m", "\033[31m" } 
};

// 前置声明
class Logger;
class LogWriter;
class LogChannel;
class LogInfo;
class LogInfoMaker;

typedef std::shared_ptr<LogInfo> LogInfo_ptr;

class LogChannel
{
public:
	LogChannel(const string &name, LogLevel level  = LDebug,
		const char* timeFormat = "%Y-%m-%d %H:%M:%S"):
		name_(name), level_(level), timeFormat_(timeFormat)
	{

	}
	virtual ~LogChannel()
	{}
	virtual void write(const LogInfo_ptr  &stream) = 0;
	const string &name() const
	{
		return name_;
	}
	LogLevel level() const
	{
		return level_;
	}
	const string &timeFormat() const
	{
		return timeFormat_;
	}
	void setLevel(LogLevel level)
	{
		level_ = level;
	}
	void setTimeFormat(const char* timeFormat)
	{
		timeFormat_ = timeFormat;
	}
		
protected:
	string name_;
	LogLevel level_;
	string timeFormat_;
};

typedef std::shared_ptr<LogChannel> LogChannel_ptr;

class LogWriter
{
public:
	LogWriter()
	{}
	virtual ~LogWriter()
	{}
	virtual void write(const LogInfo_ptr &stream) = 0;
};

typedef std::shared_ptr<LogWriter> LogWriter_ptr;

class Logger
{
public:
	friend class LogWriter;
	friend class AsyncLogWriter;
	static Logger& Instance()
	{
		static Logger *logger(new Logger());
		return *logger;
	}
	static void Destory()
	{
		printf("Logger::Destory\n");
		delete &Logger::Instance();
	}
	void add(const LogChannel_ptr &channel)
	{
		channels_[channel->name()]  = channel;
	}
	void del(const string  &name)	
	{
		auto it  = channels_.find(name);
		if(it != channels_.end())
		{
			channels_.erase(it);
		}
	}
	LogChannel_ptr get(const string& name)
	{
		auto it = channels_.find(name);
		if(it == channels_.end())
		{
			return nullptr;
		}
		return it->second;
	}
	void setWriter(const LogWriter_ptr &writer)
	{
		if(writer)
		{
			writer_  = writer;;
		}
	}
	void write(const LogInfo_ptr &stream)
	{
		if(writer_)
		{
			writer_->write(stream);
			return;
		}
		for(auto &chn : channels_)
		{
			chn.second->write(stream);
		}
	}
	void setLevel(LogLevel level)
	{
		for(auto &chn : channels_)
		{
			chn.second->setLevel(level);
		}
	}
protected:
	Logger()
	{}
	~Logger()
	{}
	// Non-copyable and non-movable
	Logger(const Logger&); // = delete;
	Logger(Logger&&); // = delete;
	Logger& operator=(const Logger&); // = delete;
	Logger& operator=(Logger&&); // = delete;
	map<string, LogChannel_ptr>  channels_;
	LogWriter_ptr writer_;
};

class LogInfo 
{
public:
	friend class LogInfoMaker;
	friend class LogChannel;
	void format(ostream& ost, const char *timeFormat = "%Y-%m-%d %H:%M:%S",
		bool enableColor  =  true, bool enableDetail = true)
	{
		if(!enableDetail && message_.str().empty())
		{
			return;	//没有任何打印信息
		}
		if(enableDetail)
		{
			//  先默认显示
			static string appName = "app";	//exeName();
#if defined(WIN32)
			ost << appName <<"(" << GetCurrentProcessId() << ") " << file_ << " " << line_ << endl;
#else
			ost << appName <<"(" << getpid() << ") " << file_ << " " << line_ << endl;
#endif
		}
		if(enableColor)
		{
			ost << COLOR[level_][1];
		}
		if(timeFormat)
		{
			// 打印当前时间
			ost << print(toLocal(ts_), timeFormat);			
		}
		ost  <<  " ["  << LogLevelStr[level_] <<  "] ";
		if(enableDetail)
		{
			ost << function_ <<  " ";
		}
		ost << message_.str();
		if(enableColor)
		{
			ost << CLEAR_COLOR;
		}
		ost << endl;
	}	
	LogLevel getLevel() const {
		return level_;
	}
	LogLevel level_;
	int line_;
	string file_;
	string function_;
	time_t ts_;
	ostringstream message_;
private:
	LogInfo(LogLevel level, const char* file, const char* function,
			int line) :
			level_(level), 
			line_(line), 
			file_(file), 
			function_(function), 
			ts_(::time(NULL)) 
	{	}	
	std::string print(const std::tm& dt, const char* fmt)
	{
		/*
#if defined(__WIN32__)
		// BOGUS hack done for VS2012: C++11 non-conformant since it SHOULD take a "const struct tm* "
		// ref. C++11 standard: ISO/IEC 14882:2011, ? 27.7.1,
		std::ostringstream oss;
		oss << std::put_time(const_cast<std::tm*>(&dt), fmt);
		return oss.str();

#else    // LINUX
*/
		const size_t size = 1024;
		char buffer[size];
		auto success = std::strftime(buffer, size, fmt, &dt);
		if (0 == success)
			return string(fmt);
		return buffer;
//#endif
	}
	std::tm toLocal(const std::time_t& time)
	{
		std::tm tm_snapshot;

#if defined(_WIN32)
		localtime_s(&tm_snapshot, &time); // thread-safe?
#else
		localtime_r(&time, &tm_snapshot); // POSIX
#endif //WIN32
		return tm_snapshot;
	}
	
};

class LogInfoMaker 
{
public:
	LogInfoMaker(LogLevel level, const char* file, 
		const char* function, int line) :
			logInfo_(new LogInfo(level, file, function, line)) 
	{
		//printf("LogInfoMaker(LogLevel level  \n");
	}
	LogInfoMaker(LogInfoMaker &&that) 
	{
		this->logInfo_ = that.logInfo_;
		that.logInfo_.reset();
	}
	LogInfoMaker(const LogInfoMaker &that) 
	{
		this->logInfo_ = that.logInfo_;
		(const_cast<LogInfoMaker &>(that)).logInfo_.reset();
	}
	~LogInfoMaker() 
	{
		*this << endl;
	}
	template<typename T>
	LogInfoMaker& operator <<(const T& data) 
	{
		if (!logInfo_) 
		{
			return *this;
		}
		logInfo_->message_ << data;
		return *this;
	}

	LogInfoMaker& operator <<(const char *data) 
	{
		if (!logInfo_) 
		{
			return *this;
		}
		if(data)
		{
			logInfo_->message_ << data;
		}
		return *this;
	}

	LogInfoMaker& operator <<(ostream&(*f)(ostream&)) 
	{
		if (!logInfo_) 
		{
			return *this;
		}
		Logger::Instance().write(logInfo_);
		logInfo_.reset();
		return *this;
	}
	void clear() 
	{
		logInfo_.reset();
	}
private:
	LogInfo_ptr logInfo_;	
};

class AsyncLogWriter;

extern "C"
{
static void* threadWrapper(void* parm);
}

void run2(int i)  
{  
	printf("test run i  = %d\n", i);
} 

class AsyncLogWriter: public LogWriter 
{
public:
	AsyncLogWriter():
		exit_flag_(false)
	{
		printf("AsyncLogWriter create\n");
		// 创建一个线程并运行
		if (int retval = pthread_create(&mId, 0, threadWrapper, this))
	    {
	        std::cerr << "Failed to spawn thread: " << retval << std::endl;
	    }
	}		 

	virtual ~AsyncLogWriter() 
	{
		printf("~AsyncLogWriter entry\n");	
		exit_flag_ = true;
		sem_.post();
		//thread_->join();
		while (pending_.size()) 
		{
			auto &next = pending_.front();
			realWrite(next);
			pending_.pop_front();
		}
		if(thread_)
		delete thread_;
		int r = pthread_join(mId, nullptr);
        if (r != 0)
        {
            cerr << "Internal error: pthread_join() returned " << r << endl;
            //        assert(0);
        }
        printf("~AsyncLogWriter exit\n");
	}
	virtual void write(const LogInfo_ptr &stream) 
	{
		{
			lock_guard<mutex> lock(mutex_);
			pending_.push_back(stream);
		}
		sem_.post();
	}
	void run() 
	{
		printf("AsyncLogWriter::run into\n");
		while (!exit_flag_) 
		{
			sem_.wait();
			{
				lock_guard<mutex> lock(mutex_);
				if (pending_.size()) {
					auto &next = pending_.front();
					realWrite(next);
					pending_.pop_front();
				}
			}
		}
		printf("AsyncLogWriter::run exit\n");
	}
	inline void realWrite(const LogInfo_ptr &stream) 
	{
		for (auto &chn : Logger::Instance().channels_) 
		{
			chn.second->write(stream);
		}
	}
protected:
	bool exit_flag_;
	//std::shared_ptr<thread> thread_;
	thread *thread_ = nullptr;
	pthread_t mId;
	deque<LogInfo_ptr> pending_;
	semaphore sem_;
	mutex mutex_;
};

static void* threadWrapper(void* parm)
{
   	printf("threadWrapper\n");
    AsyncLogWriter* t = static_cast<AsyncLogWriter*> (parm);
 
    t->run();
	return nullptr;
}


class ConsoleChannel: public LogChannel 
{
public:
	ConsoleChannel(const string& name, LogLevel level = LDebug,
			const char* timeFormat = "%Y-%m-%d %H:%M:%S") :
			LogChannel(name, level, timeFormat) 
	{	}
	virtual ~ConsoleChannel()
	{	}
	virtual void write(const LogInfo_ptr &logInfo) override
	{
		if (level() > logInfo->getLevel()) 
		{
			return;
		}
		logInfo->format(std::cout, timeFormat().c_str(), true);
	}
};

class FileChannel: public LogChannel {
public:
	FileChannel(const string& name, const string& path, LogLevel level = LDebug,
			const char* timeFormat = "%Y-%m-%d %H:%M:%S") :
			LogChannel(name, level, timeFormat), path_(path) {
	}
	virtual ~FileChannel() 
	{
		close();
	}
	virtual void write(const std::shared_ptr<LogInfo> &stream) override {
		if (level() > stream->getLevel()) {
			return;
		}
		if (!fstream_.is_open()) {
			open();
		}
		stream->format(fstream_, timeFormat().c_str(), false);
	}
	void setPath(const string& path) {
		path_ = path;
		open();
	}
	const string &path() const {
		return path_;
	}
protected:
	virtual void open() 
	{
		// Ensure a path was set
		if (path_.empty()) {
			throw runtime_error("Log file path must be set.");
		}
		// Open the file stream
		fstream_.close();
		fstream_.open(path_.c_str(), ios::out | ios::app);
		// Throw on failure
		if (!fstream_.is_open()) 
		{
			throw runtime_error("Failed to open log file: " + path_);
		}
	}
	virtual void close() {
		fstream_.close();
	}
	ofstream fstream_;
	string path_;
};

#define TraceL LogInfoMaker(LTrace, __FILE__,__FUNCTION__, __LINE__)
#define DebugL LogInfoMaker(LDebug, __FILE__,__FUNCTION__, __LINE__)
#define InfoL LogInfoMaker(LInfo, __FILE__,__FUNCTION__, __LINE__)
#define WarnL LogInfoMaker(LWarn,__FILE__, __FUNCTION__, __LINE__)
#define ErrorL LogInfoMaker(LError,__FILE__, __FUNCTION__, __LINE__)
#define FatalL LogInfoMaker(LFatal,__FILE__, __FUNCTION__, __LINE__)

}
}

#endif

