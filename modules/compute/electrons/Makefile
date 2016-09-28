IRPF90 = irpf90  #-a -d
FC     = gfortran
FCFLAGS= -O2 -ffree-line-length-none -I .
NINJA  = ninja
AR     = ar
RANLIB = ranlib

SRC=
OBJ=
LIB=-lzezfio_f -lzmq -lf77zmq
IRP_INCLUDES=-I utils_irp

ifeq ($(BUILD),1)
-include irpf90.make
export

irpf90.make: $(filter-out IRPF90_temp/%, $(wildcard */*.irp.f)) $(wildcard *.irp.f) $(wildcard *.inc.f) Makefile 
	$(IRPF90) $(IRP_INCLUDES)
else
$(error "Please run prout to build this package")
endif
