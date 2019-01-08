/*
	FreeRTOS Emu
*/

#include <juce.h>

#include <FreeRTOS.h>
#include <StackMacros.h>

#include <task.h>

#include <JUCEtask.h>
#include <JUCEtask.hpp>

static bool arrayinited = false;
static JUCETask *taskArray[MAX_JUCETASKS];
CriticalSection tasklistmutex;


void JUCETaskCreate(void (*pvTaskCode)(void * ), const signed char *pcName,
					void *pvParameters, int uxPriority, int *pxCreatedTask)
{
	tasklistmutex.enter();
	if (arrayinited != true)
	{
		for (unsigned int i = 0; i < MAX_JUCETASKS; i++)
		{
			taskArray[i] = NULL;
		}
		arrayinited = true;
	}
	unsigned int thisID = MAX_JUCETASKS;
	for (unsigned int i = 0; i < MAX_JUCETASKS; i++)
	{
		if (taskArray[i] == NULL)
		{
			thisID = i;
			break;
		}
	}
	if (thisID == MAX_JUCETASKS) return;
	JUCETask *newtask;
	String newname = (const char*) pcName;
	newtask = new JUCETask(newname);
	newtask->TaskFunction = pvTaskCode;
	newtask->parameters = pvParameters;
	newtask->iD = thisID;
	taskArray[thisID] = newtask;
	if (pxCreatedTask != NULL) *pxCreatedTask = thisID;

	newtask->startThread(uxPriority);
	tasklistmutex.exit();
}

long JUCETaskGetTickCount(void)
{
	// damn this isn't working. 
	// You can't get a pointer to the currently running thread
	// because it returns a thread class pointer
	// and these are implementad as subclasses (as it should be)
	// and upcasting is a no-no.
	// 
	// fortunately delayuntil is ignoring it anyway
	// but it prevents a 100% freertos emulation.
	// timing on PCs isn't really mS accurate anyway....
	// so who cares?
	// run core32 if you want timing!
	
	//tasklistmutex.enter();
	//JUCETask *temppointer;
	//temppointer =  Thread::getCurrentThread();
	//if (temppointer != NULL) return temppointer->tickcounter;
	return 0xffffffff;
	//tasklistmutex.exit();
}

void JUCETaskDelay(long time)
{
	JUCETask::sleep(time);
}

void JUCETaskSuspend(int taskID)
{
	tasklistmutex.enter();
	JUCETask *thistask;
	thistask = taskArray[taskID];
	tasklistmutex.exit();
	if (thistask != NULL) 
	{
		if (Thread::getCurrentThreadId() != thistask->getThreadId())
		{
			thistask->stopThread(5000);
		}
		
		else thistask->signalThreadShouldExit();
		
	}
	
}


void JUCETaskResume(int taskID)
{
	tasklistmutex.enter();
	JUCETask *thistask;
	thistask = taskArray[taskID];
	tasklistmutex.exit();
	if (thistask != NULL) thistask->startThread();
}


void JUCETaskSuspendAll(void)
{
	tasklistmutex.enter();
	JUCETask *thistaskArray[MAX_JUCETASKS];
	for (unsigned int i = 0; i < MAX_JUCETASKS; i++)
	{
		thistaskArray[i] = taskArray[i];		
	}
	tasklistmutex.exit();
	
	for (unsigned int i = 0; i < MAX_JUCETASKS; i++)
	{
		if (thistaskArray[i] != NULL)
		{
			if ( Thread::getCurrentThreadId() != thistaskArray[i]->getThreadId() )
			{
				thistaskArray[i]->stopThread(5000);
			}
			
		}
		
	}
	
}


void JUCETaskResumeAll(void)
{
	tasklistmutex.enter();
	JUCETask *thistaskArray[MAX_JUCETASKS];
	for (unsigned int i = 0; i < MAX_JUCETASKS; i++)
	{
		thistaskArray[i] = taskArray[i];		
	}
	tasklistmutex.exit();
	
	for (unsigned int i = 0; i < MAX_JUCETASKS; i++)
	{
		if (thistaskArray[i] != NULL)
		{
			thistaskArray[i]->startThread();
		}
		
	}
	
}




