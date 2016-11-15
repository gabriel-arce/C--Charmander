################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: pokedexServidor

dependents:
	-cd /home/gabriel/workspace/Operativos/commons/Debug && $(MAKE) all
	-cd /home/gabriel/workspace/Operativos/tp-2016-2c-MeQuedeSinPokebolas-/comunicacion/Debug && $(MAKE) all
	-cd /home/gabriel/workspace/Operativos/tp-2016-2c-MeQuedeSinPokebolas-/osadaFS/Debug && $(MAKE) all

# Tool invocations
pokedexServidor: $(OBJS) $(USER_OBJS) /home/gabriel/workspace/Operativos/commons/Debug/libcommons.so /home/gabriel/workspace/Operativos/tp-2016-2c-MeQuedeSinPokebolas-/comunicacion/Debug/libcomunicacion.so /home/gabriel/workspace/Operativos/tp-2016-2c-MeQuedeSinPokebolas-/osadaFS/Debug/libosadaFS.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"/home/gabriel/workspace/Operativos/tp-2016-2c-MeQuedeSinPokebolas-/osadaFS/Debug" -L"/home/gabriel/workspace/Operativos/tp-2016-2c-MeQuedeSinPokebolas-/comunicacion/Debug" -L"/home/gabriel/workspace/Operativos/commons/Debug" -o "pokedexServidor" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) pokedexServidor
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
/home/gabriel/workspace/Operativos/commons/Debug/libcommons.so:
/home/gabriel/workspace/Operativos/tp-2016-2c-MeQuedeSinPokebolas-/comunicacion/Debug/libcomunicacion.so:
/home/gabriel/workspace/Operativos/tp-2016-2c-MeQuedeSinPokebolas-/osadaFS/Debug/libosadaFS.so:

-include ../makefile.targets
