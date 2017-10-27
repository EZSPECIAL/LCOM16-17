#ifndef __TIMER_H
#define __TIMER_H

//Subscribe timer interrupts
int timer_subscribe_int(void );

//Unsubscribe timer interrupts
int timer_unsubscribe_int();

//Uses timer to sleep for "time" seconds
int timer_sleep(int time);

#endif /* __TIMER_H */
