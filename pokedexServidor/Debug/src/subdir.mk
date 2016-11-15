################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/servidorPokedex.c 

OBJS += \
./src/servidorPokedex.o 

C_DEPS += \
./src/servidorPokedex.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -D_FILE_OFFSET_BITS=64 -I"/home/gabriel/workspace/Operativos/tp-2016-2c-MeQuedeSinPokebolas-/osadaFS" -I"/home/gabriel/workspace/Operativos/tp-2016-2c-MeQuedeSinPokebolas-/comunicacion" -I"/home/gabriel/workspace/Operativos/commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


