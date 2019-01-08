$Id$

MIOS32 Tutorial #023: Exclusive access to LCD device (using a Mutex)
===============================================================================
Copyright (C) 2009 Thorsten Klose (tk@midibox.org)
Licensed for personal non-commercial use only.
All other rights reserved.
===============================================================================

Required tools:
  -> http://www.ucapps.de/mio32_c.html

===============================================================================

Required hardware:
   o MBHP_CORE_STM32 or MBHP_CORE_LPC17 or MBHP_CORE_STM32F4 or similar
   o a character or graphical LCD

===============================================================================

This tutorial explains the purpose of a Mutex.

Please read "006_rtos_tasks" if this hasn't been done before to understand
the purpose of a Task.


Two independent tasks are writing to the LCD screen.
  - TASK_LCD1(), which is running at low priority to print a vertical bar
    on the upper line. For demonstration purposes, the execution of this
    task takes much longer than 1 mS (the bar is print 100 times) before
    the task yields.
  - TASK_LCD2(), which is running at higher priority to print the current
    System time each millisecond on the lower line.

If accesses to the LCD wouldn't be protected with a Semaphore, following
scenario could happen very quickly:

  - TASK_LCD1() sets the cursor to the upper line
  - TASK_LCD1() starts to print the first characters
  - after at least 1 mS, TASK_LCD2() will interrupt TASK_LCD1()
  - TASK_LCD2() sets the cursor to the lower line
  - TASK_LCD2() prints the system time and waits for the next tick
  - TASK_LCD1() continues
  -> it will print the remaining characters behind the System Time
     (cursor is at wrong position)

But even more mysterious effects could happen sporadically, e.g. if MIOS32_LCD
low-level routines are interrupted during a write access to the LCD device,
like missed write accesses or falsified characters.

Note: since FreeRTOS tasks are preemptive, this issue could even happen if 
both tasks are running at same priority!

---

Now you could say, that this issue can be easily solved by disabling the
interruption of a task, e.g. with the FreeRTOS functions:
  portENTER_CRITICAL();
and
  portEXIT_CRITICAL();

But this would lead to the effect, that *all* tasks will be disabled
between these function calls. This can affect the responsiveness of your
application dramatically (e.g. received MIDI events will be delayed).

---

Then you could say: let's use a global variable to control the access.
Set it to 1 when any task should write to the LCD, set back to 0
when LCD output is finished. 
All LCD output tasks have to check if the variable is 0 before setting
it to take over the control.

No bad idea, but this method won't work once tasks are running at
different priorities.

Example:
  - TASK_LCD1() (low-priority) sets variable to 1
  - TASK_LCD2() (high-priority) interrupts TASK_LCD1()
  - TASK_LCD2() has to wait until the variable has been set back to 0

But since TASK_LCD2() is running at high priority, and accordingly blocks
all lower priority tasks, TASK_LCD1() won't be continued as long as TASK_LCD2()
is waiting!

---

A much better method is the usage of a Mutex (a special type of Semaphore)
to control the access to the resource (LCD device).
A nice explanation can be found under:
  http://www.freertos.org/index.html?http://www.freertos.org/Inter-Task-Communication.html#Mutexes

Quote: "When used for mutual exclusion the mutex acts like a token that is used
to guard a resource. When a task wishes to access the resource it must first 
obtain ('take') the token. When it has finished with the resource it must 
'give' the token back - allowing other tasks the opportunity to access the
same resource."


Main advantage:
Quote: "If a high priority task blocks while attempting to obtain a mutex (token) 
that is currently held by a lower priority task, then the priority of the task 
holding the token is temporarily raised to that of the blocking task.
This mechanism is designed to ensure the higher priority task is kept in the
blocked state for the shortest time possible, and in so doing minimise the
'priority inversion' that has already occurred."


In short: use a Mutex whenever tasks have to share a resource!
Use a separate Mutex for each resource!
It's simple and doesn't hurt! :-)


In my own application, I mostly define two macros to take/give the Mutex
as it can be seen in the header of the app.c file:
-------------------------------------------------------------------------------
xSemaphoreHandle xLCDSemaphore;
#define MUTEX_LCD_TAKE { while( xSemaphoreTakeRecursive(xLCDSemaphore, (portTickType)1) != pdTRUE ); }
#define MUTEX_LCD_GIVE { xSemaphoreGiveRecursive(xLCDSemaphore); }
-------------------------------------------------------------------------------

This has the advantage, that the Mutex handling can be easily adapted to
other operating systems (-> emulation) if required.
It also improves the readability.

The Mutex is initialized in APP_Init():
-------------------------------------------------------------------------------
  // create Mutex for LCD access
  xLCDSemaphore = xSemaphoreCreateRecursiveMutex();
-------------------------------------------------------------------------------

Note that a recursive Mutex is used, which will allow nested calls.
Although this isn't required here, I propose this as the prefered solution.
Without such a Mutex, an application will hang up once a Mutex is taken
two times (e.g. from a subroutine, called by another subroutine which both
take the Mutex at the beginning) - you don't need to consider such dangers 
with the recursive mechanism.


Whenever a routine wants to access the resource (the LCD), just write:
-------------------------------------------------------------------------------
    // request LCD access
    MUTEX_LCD_TAKE;
-------------------------------------------------------------------------------

and whenever the routine is finished, write:

-------------------------------------------------------------------------------
    // release LCD access for other tasks
    MUTEX_LCD_GIVE;
-------------------------------------------------------------------------------

thats all!


Exercise
--------

Try what happens with following macro definitions:
-------------------------------------------------------------------------------
#define MUTEX_LCD_TAKE { }
#define MUTEX_LCD_GIVE { }
-------------------------------------------------------------------------------
(no Mutex used to access the LCD)


WARNING
-------

It isn't allowed to use a Semaphores/Mutex:
  - by APP_Init(), as this function is called before FreeRTOS is started

  - by APP_Background(), because the idle task should never block!
    Solution: define your own task at (tskIDLE_PRIORITY + 0)

  - by interrupt service routines, e.g. called by MIOS32_Timer, but also
    APP_SRIO_ServicePrepare() and APP_SRIO_ServiceFinished(), as they are 
    running outside the FreeRTOS context

    Solution: create a task as a "gateway" to the resource.
    Use xSemaphoreGiveFromISR() or xQueue*ISR() functions to communicate
    with the task.
    See also
      http://www.freertos.org/index.html?http://www.freertos.org/a00113.html
    and
      http://www.freertos.org/index.html?http://www.freertos.org/a00018.html

===============================================================================
