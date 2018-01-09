#include <iostream>
#include <unistd.h>

#include "logger.hpp"
using namespace std;
using namespace ZL::Util;


void run(int i)  
{  printf("test run i  = %d\n", i);
} 

int main(void)
{
	//设置日志系统
	Logger::Instance().add(std::make_shared<ConsoleChannel>("stdout", LTrace));
	Logger::Instance().setWriter(std::make_shared<AsyncLogWriter>());

	TraceL << "test" << endl; 
	DebugL << "test" << endl; 
	InfoL << "test" << endl; 
	WarnL << "test" << endl; 
	ErrorL << "test" << endl; 
	FatalL << "test" << endl; 
	sleep(1);
	printf("change log level to LWarn\n");
	Logger::Instance().setLevel(LWarn);
	TraceL << "test2" << endl; 
	DebugL << "test2" << endl; 
	InfoL << "test2" << endl; 
	WarnL << "test2" << endl; 
	ErrorL << "test2" << endl; 
	FatalL << "test2" << endl; 

	Logger::Destory();
	return 0;
}
