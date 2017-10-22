################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/main.c \
../src/syscalls.c \
../src/system_stm32f10x.c 

OBJS += \
./src/main.o \
./src/syscalls.o \
./src/system_stm32f10x.o 

C_DEPS += \
./src/main.d \
./src/syscalls.d \
./src/system_stm32f10x.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -DSTM32F1 -DSTM32F103C8Tx -DSTM32 -DDEBUG -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD -I"C:/Users/piotr/Desktop/micromouse/inc" -I"C:/Users/piotr/Desktop/micromouse/CMSIS/core" -I"C:/Users/piotr/Desktop/micromouse/CMSIS/device" -I"C:/Users/piotr/Desktop/micromouse/StdPeriph_Driver/inc" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


