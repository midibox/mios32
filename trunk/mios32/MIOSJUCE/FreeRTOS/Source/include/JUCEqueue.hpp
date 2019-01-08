/*
	FreeRTOS Emu
*/

#ifndef JUCEQUEUE_HPP
#define JUCEQUEUE_HPP



void MutexList_Add(CriticalSection *mutexPointer, int mutexID);

void MutexList_Del(CriticalSection *mutexPointer);


CriticalSection *MutexList_GetPointer(int mutexID);

int MutexList_GetID(CriticalSection *mutexPointer);


typedef struct MutexList_t
{
	int mutexID;
	CriticalSection *mutexPointer;
	MutexList_t *next;
} MutexList_t;



#endif /* JUCEQUEUE_HPP */



