/*
 * Copyright 2016-2018 NXP Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
/**
 * @file    180910_alarm_queue.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"

/* TODO: insert other include files here. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"
#include "queue.h"


/* TODO: insert other definitions and declarations here. */
#define PRINTF_MIN_STACK (110)
#define GET_ARGS(args,type) *((type*)args)
#define EVENT_SECONDS (1<<0)
#define EVENT_MINUTES (1<<1)
#define EVENT_HOURS   (1<<2)

//#define FIRST60SCOUNTFLAG_STATE_H 1

typedef struct
{
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
}alarm_t;

alarm_t alarm;

typedef struct
{
	SemaphoreHandle_t minutes_semaphore;
	SemaphoreHandle_t hours_semaphore;
	EventGroupHandle_t event_alarm_signal;
	SemaphoreHandle_t serial_port_mutex;
	QueueHandle_t mailbox;
}task_args_t;

//estructura para los mensajes en el mailbox
typedef enum {	seconds_type,	minutes_type,	hours_type } time_types_t;

typedef struct
{
	time_types_t time_type;
	uint8_t value;
}time_msg_t;

/*
 * @brief   Application entry point.
 */

void seconds_task(void*args)
{
	task_args_t task_args = GET_ARGS(args,task_args_t);
	uint8_t seconds  = 0;
	TickType_t last_wake_time = xTaskGetTickCount();
	time_msg_t msg;
	time_msg_t *pmsg;
//	uint8_t first60sCount_flag = 1;
	msg.time_type = seconds_type;
	for(;;)
	{
		if (alarm.second == seconds)
		{
			xEventGroupSetBits(task_args.event_alarm_signal, EVENT_SECONDS);
		}

		vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1000));

		seconds++;

		if (60 == seconds)
		{
			seconds = 0;
			xSemaphoreGive(task_args.minutes_semaphore);
		}

		msg.value = seconds;
		pmsg = pvPortMalloc(sizeof(time_msg_t));
		*pmsg = msg;
		xQueueSend(task_args.mailbox,&pmsg,portMAX_DELAY);


	}
}

void minutes_task(void*args)
{
	task_args_t task_args = GET_ARGS(args,task_args_t);
	uint8_t minutes  = 0;
	time_msg_t msg;
	time_msg_t *pmsg;
	msg.time_type = minutes_type;
	for(;;)
	{
		if (alarm.minute == minutes)
		{
			xEventGroupSetBits(task_args.event_alarm_signal, EVENT_MINUTES);
		}

		xSemaphoreTake(task_args.minutes_semaphore,portMAX_DELAY);
		minutes++;

		if (60 == minutes)
		{
			minutes = 0;
			xSemaphoreGive(task_args.hours_semaphore);
		}

		msg.value = minutes;
		pmsg = pvPortMalloc(sizeof(time_msg_t));
		*pmsg = msg;
		xQueueSend(task_args.mailbox,&pmsg,portMAX_DELAY);
	}
}

void hours_task(void*args)
{
	task_args_t task_args = GET_ARGS(args,task_args_t);
	uint8_t hours  = 0;
	time_msg_t msg;
	time_msg_t *pmsg;
	msg.time_type = hours_type;
	for(;;)
	{
		if (alarm.hour == hours)
		{
			xEventGroupSetBits(task_args.event_alarm_signal, EVENT_HOURS);
		}

		xSemaphoreTake(task_args.hours_semaphore,portMAX_DELAY);
		hours++;

		if (24 == hours)
		{
			hours = 0;
		}

		msg.value = hours;
		pmsg = pvPortMalloc(sizeof(time_msg_t));
		*pmsg = msg;
		xQueueSend(task_args.mailbox,&pmsg,portMAX_DELAY);
	}
}

void alarm_task(void*args)
{
	task_args_t task_args = GET_ARGS(args,task_args_t);
	for(;;)
	{
		xEventGroupWaitBits(task_args.event_alarm_signal,EVENT_HOURS|EVENT_MINUTES|EVENT_SECONDS, pdTRUE, pdTRUE, portMAX_DELAY);
		xSemaphoreTake(task_args.serial_port_mutex,portMAX_DELAY);
		PRINTF("\rALARM\n");
		xSemaphoreGive(task_args.serial_port_mutex);
	}
}

void print_task(void*args)
{
	task_args_t task_args = GET_ARGS(args,task_args_t);
	time_msg_t *received_msg;
	uint8_t seconds = 0;
	uint8_t minutes = 0;
	uint8_t hours   = 0;
	for(;;)
	{
		xQueueReceive(task_args.mailbox,&received_msg,portMAX_DELAY);

		xSemaphoreTake(task_args.serial_port_mutex,portMAX_DELAY);

		switch (received_msg->time_type) {
		case seconds_type:
			seconds = received_msg->value;
			break;
		case minutes_type:
			minutes = received_msg->value;
			break;
		case hours_type:
			hours   = received_msg->value;
			break;
		default:
			PRINTF("\rError\n");
			break;
		}

		PRINTF("\r%2i:%2i:%2i\n", hours, minutes, seconds);
		vPortFree(received_msg);
		xSemaphoreGive(task_args.serial_port_mutex);
	}
}

int main(void) {

	static task_args_t args;
	args.event_alarm_signal = xEventGroupCreate();
	args.mailbox = xQueueCreate(3,sizeof(time_msg_t*));
	args.minutes_semaphore = xSemaphoreCreateBinary();
	args.hours_semaphore = xSemaphoreCreateBinary();
	args.serial_port_mutex = xSemaphoreCreateMutex();

  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
  	/* Init FSL debug console. */
    BOARD_InitDebugConsole();

    alarm.hour = 0;
    alarm.minute = 1;
    alarm.second = 5;

//    PRINTF("Hello World\n");
    //TODO verificar (void*)&args
    //TODO configMAX_PRIORITIES
    xTaskCreate(seconds_task, "seconds", configMINIMAL_STACK_SIZE, (void*)&args, configMAX_PRIORITIES-3, NULL);
    xTaskCreate(minutes_task, "minutes", configMINIMAL_STACK_SIZE, (void*)&args, configMAX_PRIORITIES-2, NULL);
    xTaskCreate(hours_task,   "hours",   configMINIMAL_STACK_SIZE, (void*)&args, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(alarm_task,   "alarm",   PRINTF_MIN_STACK,         (void*)&args, configMAX_PRIORITIES-4, NULL);
    xTaskCreate(print_task,   "print",   PRINTF_MIN_STACK,         (void*)&args, configMAX_PRIORITIES-4, NULL);
    vTaskStartScheduler();

    /* Enter an infinite loop, just incrementing a counter. */
    while(1) {

    }
    return 0 ;
}
