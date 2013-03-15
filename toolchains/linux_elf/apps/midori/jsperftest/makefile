all: zoog.dis posix.dis

clean:
	rm -f *.dis *.S

%: %.c
	gcc -o $@ -g $^

%.S: %.hex
	./hextoasm.py $^

%.o: %.S
	gcc -c -o $@ -g $^

%.dis: %.o
	objdump --disassemble $^ > $@
