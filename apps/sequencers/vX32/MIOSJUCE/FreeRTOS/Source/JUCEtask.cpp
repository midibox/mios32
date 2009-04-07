/*
	FreeRTOS Emu
*/

#include <juce.h>

#include <FreeRTOS.h>
#include <StackMacros.h>

#include <task.h>

#include <JUCEtask.h>
#include <JUCEtask.hpp>


static taskList_t* TaskList;
CriticalSection tasklistmutex;


void JUCETaskCreate(void (*pvTaskCode)(void * ), char *pcName, 
					void *pvParameters, int uxPriority, int *pxCreatedTask)
{
	JUCETask *newtask;
	String newname = pcName;
	newtask = new JUCETask(newname);
	newtask->TaskFunction = pvTaskCode;
	newtask->parameters = pvParameters;
	*pxCreatedTask = newtask->getThreadId();
	
	newtask->startThread(uxPriority);
}

long JUCETaskGetTickCount(void)
{
	int tempID = JUCETask::getCurrentThreadId();
	JUCETask *temppointer;
	temppointer = TaskList_GetPointer(tempID);
	if (temppointer != NULL) return temppointer->tickcounter;
	return 0xffffffff;
}

void JUCETaskDelay(long time)
{
	JUCETask::sleep(time);
}

void JUCETaskSuspend(int taskID)
{
	(TaskList_GetPointer(taskID))->wait(-1);
}


void JUCETaskResume(int taskID)
{
	(TaskList_GetPointer(taskID))->notify();
}


void JUCETaskSuspendAll(void)
{
	taskList_t *temp = TaskList;	
	while (temp != NULL)
	{
		temp->taskPointer->wait(-1);
	}
	
}


void JUCETaskResumeAll(void)
{
	taskList_t *temp = TaskList;	
	while (temp != NULL)
	{
		temp->taskPointer->notify();
	}
	
}




void TaskList_Add(JUCETask *taskPointer, int taskID)
{
	tasklistmutex.enter();
	taskList_t *root = TaskList;
	taskList_t *newentry = new taskList_t;
	newentry->taskID = taskID;
	newentry->taskPointer = taskPointer;
	newentry->next = root;
	TaskList = newentry;
	tasklistmutex.exit();
}

void TaskList_Del(JUCETask *taskPointer)
{
	tasklistmutex.enter();
	taskList_t *temp = TaskList;
	taskList_t *prev = NULL;
	while (temp->taskPointer != taskPointer)
	{
		prev = temp;
		temp = temp->next;
	}
	
	if (prev == NULL) 
	{
		TaskList = temp->next;
	} else {
		prev->next = temp->next;
	}
	
	delete temp;
	tasklistmutex.exit();
	
}


JUCETask *TaskList_GetPointer(int taskID)
{
	tasklistmutex.enter();
	taskList_t *temp = TaskList;
	JUCETask *returnPointer = NULL;
	
	while (temp != NULL)
	{
		if (temp->taskID == taskID)
		{
			returnPointer = temp->taskPointer;
			break;
		}
		
		temp = temp->next;
	}
	
	tasklistmutex.exit();
	return returnPointer;
	
}

int TaskList_GetID(JUCETask *taskPointer)
{
	tasklistmutex.enter();
	taskList_t *temp = TaskList;
	int returnID = -1;
	
	while (temp != NULL)
	{
		if (temp->taskPointer == taskPointer)
		{
			returnID = temp->taskID;
			break;
		}
		
		temp = temp->next;
	}
	tasklistmutex.exit();
	
	return returnID;
}




JUCETask::JUCETask(const String& threadName) : Thread(threadName)
{
	TaskList_Add(this, getThreadId());
}
	
JUCETask::~JUCETask()
{
	TaskList_Del(this);
	// allow the thread 2 seconds to stop cleanly - should be plenty of time.
	stopThread (2000);
}
