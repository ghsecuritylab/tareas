/*
 * Copyright (c) 2017, NXP Semiconductor, Inc.
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
 * @file    hw3.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "event_groups.h"
#include "queue.h"

#define TYPE_A

#ifdef TYPE_A
#define PRODUCER_PRIORITY 		(configMAX_PRIORITIES)
#define CONSUMER_PRIORITY 		(configMAX_PRIORITIES-1)
#define SUPERVISOR_PRIORITY 	(configMAX_PRIORITIES-2)
#define PRINTER_PRIORITY 		(configMAX_PRIORITIES-3)
#define TIMER_PRIORITY 			(configMAX_PRIORITIES-4)
#else
#define PRODUCER_PRIORITY 		(configMAX_PRIORITIES-4)
#define CONSUMER_PRIORITY 		(configMAX_PRIORITIES-3)
#define SUPERVISOR_PRIORITY 	(configMAX_PRIORITIES-2)
#define PRINTER_PRIORITY 		(configMAX_PRIORITIES-2)
#define TIMER_PRIORITY 			(configMAX_PRIORITIES)
#endif

#define PRINTF_MIN_STACK (110)
#define GET_ARGS(args,type) *((type*)args)
#define EVENT_CONSUMER (1<<0)
#define EVENT_PRODUCER (1<<1)

typedef struct
{
	int8_t shared_memory;
	SemaphoreHandle_t seconds_signal;
	EventGroupHandle_t supervisor_signals;
	SemaphoreHandle_t serial_port_mutex;
	SemaphoreHandle_t shared_memory_mutex;
	QueueHandle_t mailbox;
}task_args_t;

typedef enum {consumer_id,producer_id,supervisor_id} id_t;

typedef struct
{
	id_t id;
	uint32_t data;
	const char * msg;
}msg_t;

void task_producer(void*args);
void task_consumer(void*args);
void task_supervisor(void*args);
void task_printer(void*args);
void task_timer(void*args);

void task_producer(void*args)
{
	const char producer_msg[] = "Producer task produced successfully";
	msg_t msg;
	msg.id = producer_id;
	msg.msg = producer_msg;
	task_args_t task_args = GET_ARGS(args,task_args_t);
	for(;;)
	{
		xSemaphoreTake(task_args.seconds_signal,portMAX_DELAY);

		xSemaphoreTake(task_args.shared_memory_mutex,portMAX_DELAY);
		task_args.shared_memory++;
		msg.data = task_args.shared_memory;
		xSemaphoreGive(task_args.shared_memory_mutex);

		xQueueSend(task_args.mailbox,&msg,portMAX_DELAY);

		xEventGroupSetBits(task_args.supervisor_signals, EVENT_PRODUCER);


	}
}

void task_consumer(void*args)
{
	const char consumer_msg[] = "Consumer task consumed successfully";
	task_args_t task_args = GET_ARGS(args,task_args_t);
	msg_t msg;
	msg.id = consumer_id;
	msg.msg = consumer_msg;
	for(;;)
	{
		xSemaphoreTake(task_args.shared_memory_mutex,portMAX_DELAY);
		task_args.shared_memory--;
		msg.data = task_args.shared_memory;

		xQueueSend(task_args.mailbox,&msg,portMAX_DELAY);

		xSemaphoreGive(task_args.shared_memory_mutex);

		xEventGroupSetBits(task_args.supervisor_signals, EVENT_CONSUMER);

		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

void task_supervisor(void*args)
{
	const char consumer_msg[] = "Supervisor task supervised successfully";
	task_args_t task_args = GET_ARGS(args,task_args_t);
	msg_t msg;
	uint8_t supervise_count = 0;
	msg.id = supervisor_id;
	msg.msg = consumer_msg;
	for(;;)
	{
		xEventGroupWaitBits(task_args.supervisor_signals, EVENT_CONSUMER|EVENT_PRODUCER, pdTRUE, pdTRUE, portMAX_DELAY);

		supervise_count++;

		if(0==supervise_count%10)
		{
			msg.data = supervise_count;
			xQueueSend(task_args.mailbox,&msg,portMAX_DELAY);
		}
	}
}

void task_printer(void*args)
{
	task_args_t task_args = GET_ARGS(args,task_args_t);
	msg_t received_msg;
	for(;;)
	{
		xQueueReceive(task_args.mailbox,&received_msg,portMAX_DELAY);

		xSemaphoreTake(task_args.serial_port_mutex,portMAX_DELAY);
		switch(received_msg.id)
		{
		case producer_id:
			PRINTF("\rProducer sent:");
			PRINTF(received_msg.msg);
			PRINTF(" |DATA: %i\n",received_msg.data);
			break;
		case consumer_id:
			PRINTF("\rConsumer sent:");
			PRINTF(received_msg.msg);
			PRINTF(" |DATA: %i\n",received_msg.data);
			break;
		case supervisor_id:
			PRINTF("\rSupervisor sent:");
			PRINTF(received_msg.msg);
			PRINTF(" |DATA: %i\n",received_msg.data);
			break;
		default:
			PRINTF("\rError\n");
			break;
		}
		xSemaphoreGive(task_args.serial_port_mutex);
	}
}

void task_timer(void*args)
{
	task_args_t task_args = GET_ARGS(args,task_args_t);
	uint32_t seconds  = 0;
	TickType_t last_wake_time = xTaskGetTickCount();
	for(;;)
	{
		seconds++;
		xSemaphoreTake(task_args.serial_port_mutex,portMAX_DELAY);
		PRINTF("\rTime: %i seconds since reset\n",seconds);
		xSemaphoreGive(task_args.serial_port_mutex);

		xSemaphoreGive(task_args.seconds_signal);

		vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1000));
	}
}

int main(void)
{
	static task_args_t args;
	args.supervisor_signals = xEventGroupCreate();
	args.seconds_signal = xSemaphoreCreateBinary();
	args.mailbox = xQueueCreate(5,sizeof(msg_t));
	args.serial_port_mutex = xSemaphoreCreateMutex();
	args.shared_memory_mutex = xSemaphoreCreateMutex();

	srand(0x15458523);

	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	BOARD_InitDebugConsole();

	xTaskCreate(task_producer, "producer", PRINTF_MIN_STACK, (void*)&args, PRODUCER_PRIORITY, NULL);
	xTaskCreate(task_consumer, "consumer", PRINTF_MIN_STACK, (void*)&args, CONSUMER_PRIORITY, NULL);
	xTaskCreate(task_supervisor, "supervisor", PRINTF_MIN_STACK, (void*)&args, SUPERVISOR_PRIORITY, NULL);
	xTaskCreate(task_printer, "printer", PRINTF_MIN_STACK, (void*)&args, PRINTER_PRIORITY, NULL);
	xTaskCreate(task_timer, "timer", PRINTF_MIN_STACK, (void*)&args, TIMER_PRIORITY, NULL);
	vTaskStartScheduler();

	while(1) {}

	return 0 ;
}
