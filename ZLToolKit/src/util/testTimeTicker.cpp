#include <iostream>
#include <unistd.h>

#include "TimeTicker.hpp"
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
	Ticker *timeTicker =  new Ticker(3000);//计时器

	DebugL << "start async task"<< endl;
	timeTicker->resetTime();//开始计时

	sleep(2);
	
	//这时应该消耗了极短的时间，并没有阻塞当前主线程，timeTicker.elapsedTime()的结果应该接近0毫秒
	DebugL << "async task time1:" <<  timeTicker->elapsedTime() << "ms" << endl;
	sleep(1);
	
	//这时应该消耗了极短的时间，并没有阻塞当前主线程，timeTicker.elapsedTime()的结果应该接近0毫秒
	DebugL << "async task time2:" <<  timeTicker->elapsedTime() << "ms" << endl;
	
	delete timeTicker;

	SmoothTicker smoothTicker;
	DebugL << "start smoothTicker task"<< endl;
	sleep(1);
	DebugL << "async smoothTicker time1:" <<  smoothTicker.elapsedTime() << "ms" << endl;
	sleep(3);
	DebugL << "async smoothTicker time2:" <<  smoothTicker.elapsedTime() << "ms" << endl;
	sleep(2);
	DebugL << "async smoothTicker time3:" <<  smoothTicker.elapsedTime() << "ms" << endl;
	Logger::Destory();
	return 0;
}
