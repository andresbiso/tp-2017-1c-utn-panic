################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/CapaMemoria.c \
../src/Kernel.c \
../src/estados.c \
../src/inotify.c 

OBJS += \
./src/CapaMemoria.o \
./src/Kernel.o \
./src/estados.o \
./src/inotify.o 

C_DEPS += \
./src/CapaMemoria.d \
./src/Kernel.d \
./src/estados.d \
./src/inotify.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2017-1c-utn-panic/PANICommons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


