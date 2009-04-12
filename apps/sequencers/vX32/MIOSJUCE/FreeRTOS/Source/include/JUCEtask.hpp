/*
	FreeRTOS Emu
*/

#ifndef JUCETASK_HPP
#define JUCETASK_HPP

#define MAX_JUCETASKS ((unsigned int)20)



class JUCETask : public Thread ,
				 public Timer
{
public:
	void *parameters;
	void (*TaskFunction)(void *);
	long tickcounter;
	int iD;

	JUCETask(const String& threadName) : Thread(threadName)
	{
	}

	~JUCETask()
	{
		// allow the thread 4 seconds to stop cleanly - should be plenty of time.
		stopThread (4000);
	}

	void timerCallback()
    {
		tickcounter++;
	}

	void suspend()
	{
		wait(-1);
	}
	
	void resume()
	{
		notify();
	}

	void run()
	{
		startTimer(1);
		// this is the code that runs this thread - we'll loop continuously,
		// updating the co-ordinates of our blob.

		// threadShouldExit() returns true when the stopThread() method has been
		// called, so we should check it often, and exit as soon as it gets flagged.
		while (! threadShouldExit())
		{
			TaskFunction(parameters);
			// sleep a bit so the threads don't all grind the CPU to a halt..
			wait (1);

		}

	}

};



#endif /* JUCETASK_HPP */



