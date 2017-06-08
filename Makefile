HEADERS = tinyFS.h TinyFS_errno.h libDisk.h
OBJECTS = tinyFsDemo.o libDisk.o libTinyFS.o

default: tinyFsDemo

%.o: %.c $(HEADERS)
	gcc -c $< -o $@

tinyFsDemo: $(OBJECTS)
	gcc $(OBJECTS) -o $@

clean:
	-rm -f $(OBJECTS)
	-rm -f tinyFsDemo
