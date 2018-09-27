################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../amazon-freertos/portable/fsl_tickless_systick.c \
../amazon-freertos/portable/heap_4.c 

OBJS += \
./amazon-freertos/portable/fsl_tickless_systick.o \
./amazon-freertos/portable/heap_4.o 

C_DEPS += \
./amazon-freertos/portable/fsl_tickless_systick.d \
./amazon-freertos/portable/heap_4.d 


# Each subdirectory must supply rules for building sources it contributes
amazon-freertos/portable/%.o: ../amazon-freertos/portable/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DSDK_OS_BAREMETAL -DFSL_RTOS_BM -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DSDK_OS_FREE_RTOS -DFSL_RTOS_FREE_RTOS -DCPU_MK64FN1M0VDC12_cm4 -DCPU_MK64FN1M0VDC12 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I../board -I../source -I../ -I../amazon-freertos/FreeRTOS/portable -I../amazon-freertos/portable -I../amazon-freertos/include -I../drivers -I../CMSIS -I../utilities -I../startup -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


