/*
	FreeRTOS Emu
*/

#ifndef JUCETASK_HPP
#define JUCETASK_HPP





class JUCETask : public Thread ,
				 public Timer
{
public:
	void *parameters;
	void (*TaskFunction)(void *);
	long tickcounter;
	
	JUCETask(const String& threadName);
	
	~JUCETask();
	
	void timerCallback()
    {
		tickcounter++;
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



void TaskList_Add(JUCETask *taskPointer, int taskID);

void TaskList_Del(JUCETask *taskPointer);


JUCETask *TaskList_GetPointer(int taskID);

int TaskList_GetID(JUCETask *taskPointer);


typedef struct taskList_t
{
	int taskID;
	JUCETask *taskPointer;
	taskList_t *next;
} taskList_t;




#endif /* JUCETASK_HPP */



