/*
	FreeRTOS Emu
*/

#ifndef JUCETIMER_HPP
#define JUCETIMER_HPP





class JUCETimer : public Timer
{
private:
	void (*TimerFunction)();
	
public:
	JUCETimer(void (*_irq_handler)())
	{
		TimerFunction = _irq_handler;
	}
	
	~JUCETimer()
	{
		stopTimer();
	}
	
	void timerCallback()
	{
		TimerFunction();
	}
	
};




#endif /* JUCETIMER_HPP */


