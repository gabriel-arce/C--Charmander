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
	-cd /home/utnso/git/tp-2016-2c-MeQuedeSinPokebolas-/comunicacion/Debug && $(MAKE) clean all
	-cd /home/utnso/git/tp-2016-2c-MeQuedeSinPokebolas-/osadaFS/Debug && $(MAKE) clean all

# Tool invocations
pokedexServidor: $(OBJS) $(USER_OBJS) /home/utnso/git/tp-2016-2c-MeQuedeSinPokebolas-/comunicacion/Debug/libcomunicacion.so /home/utnso/git/tp-2016-2c-MeQuedeSinPokebolas-/osadaFS/Debug/libosadaFS.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"/home/utnso/git/tp-2016-2c-MeQuedeSinPokebolas-/osadaFS/Debug" -L"/home/utnso/git/tp-2016-2c-MeQuedeSinPokebolas-/comunicacion/Debug" -o "pokedexServidor" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) pokedexServidor
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
/home/utnso/git/tp-2016-2c-MeQuedeSinPokebolas-/comunicacion/Debug/libcomunicacion.so:
/home/utnso/git/tp-2016-2c-MeQuedeSinPokebolas-/osadaFS/Debug/libosadaFS.so:

-include ../makefile.targets
