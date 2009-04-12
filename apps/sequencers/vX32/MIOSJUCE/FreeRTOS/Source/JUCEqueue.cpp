/*
	FreeRTOS Emu
*/

#include <juce.h>

#include <FreeRTOS.h>
#include <StackMacros.h>

#include <semphr.h>
#include <queue.h>

#include <JUCEqueue.h>
#include <JUCEqueue.hpp>


static MutexList_t *MutexList = NULL;
CriticalSection queuelistmutex;
int mutexID = 0;

xSemaphoreHandle JUCESemaphoreCreateMutex(void)
{
	int returnID = mutexID++;
	CriticalSection *mutexPointer = new CriticalSection;
	MutexList_Add(mutexPointer, returnID);
	return returnID;
}

int JUCESemaphoreTake(xSemaphoreHandle Semaphore)
{
	if (MutexList_GetPointer(Semaphore) != NULL)
	{
		return (MutexList_GetPointer(Semaphore))->tryEnter();
	}
	return 0;
}

int JUCESemaphoreGive(xSemaphoreHandle Semaphore)
{
	if (MutexList_GetPointer(Semaphore) != NULL)
	{
		(MutexList_GetPointer(Semaphore))->exit();
		return 1;
	}
	return 0;
}



void MutexList_Add(CriticalSection *mutexPointer, int mutexID)
{
	queuelistmutex.enter();
	MutexList_t *root = MutexList;
	MutexList_t *newentry = new MutexList_t;
	newentry->mutexID = mutexID;
	newentry->mutexPointer = mutexPointer;
	newentry->next = root;
	MutexList = newentry;
	queuelistmutex.exit();
}

void MutexList_Del(CriticalSection *mutexPointer)
{
	queuelistmutex.enter();
	if (mutexPointer != NULL &&
		MutexList != NULL) 
	{
		MutexList_t *temp = MutexList;
		MutexList_t *prev = NULL;
		
		while (temp != NULL)
		{
			if (temp->mutexPointer == mutexPointer)
			{
				break;
			}
			prev = temp;
			temp = temp->next;
		}
		
		if (prev == NULL) 
		{
			MutexList = temp->next;
		} else {
			prev->next = temp->next;
		}
		
		delete temp;
		queuelistmutex.exit();
	}
}

CriticalSection *MutexList_GetPointer(int mutexID)
{
	queuelistmutex.enter();
	MutexList_t *temp = MutexList;
	CriticalSection *returnPointer = NULL;
	
	while (temp != NULL)
	{
		if (temp->mutexID == mutexID)
		{
			returnPointer = temp->mutexPointer;
			break;
		}
		
		temp = temp->next;
	}
	
	queuelistmutex.exit();
	return returnPointer;
	
}

int MutexListList_GetID(CriticalSection *mutexPointer)
{
	queuelistmutex.enter();
	MutexList_t *temp = MutexList;
	int returnID = -1;
	
	while (temp != NULL)
	{
		if (temp->mutexPointer == mutexPointer)
		{
			returnID = temp->mutexID;
			break;
		}
		
		temp = temp->next;
	}
	queuelistmutex.exit();
	
	return returnID;
}

