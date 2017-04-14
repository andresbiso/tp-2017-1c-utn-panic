################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../panicommons/panilogger.c \
../panicommons/panisocket.c 

OBJS += \
./panicommons/panilogger.o \
./panicommons/panisocket.o 

C_DEPS += \
./panicommons/panilogger.d \
./panicommons/panisocket.d 


# Each subdirectory must supply rules for building sources it contributes
panicommons/%.o: ../panicommons/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


