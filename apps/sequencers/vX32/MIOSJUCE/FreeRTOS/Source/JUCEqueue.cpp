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


static MutexList_t *MutexList;
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
	return (MutexList_GetPointer(Semaphore))->tryEnter();
	
}

int JUCESemaphoreGive(xSemaphoreHandle Semaphore)
{
	(MutexList_GetPointer(Semaphore))->exit();
	return 1;
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
	MutexList_t *temp = MutexList;
	MutexList_t *prev = NULL;
	while (temp->mutexPointer != mutexPointer)
	{
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

