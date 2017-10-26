#obj-m += dht11.o

obj-m += industrialio.o
industrialio-y := industrialio-core.o industrialio-event.o inkern.o
industrialio-y += industrialio-buffer.o
industrialio-y += industrialio-trigger.o
industrialio-y += buffer_cb.o

obj-m += industrialio-core.o
obj-m += industrialio-event.o
obj-m += inkern.o
obj-m += industrialio-buffer.o
obj-m += industrialio-trigger.o
obj-m += industrialio-triggered-buffer.o
obj-m += kfifo_buf.o

clean:
	-rm -f *.o *.mod.c *.order *.symvers

