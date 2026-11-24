vivefilter.so: vivefilter.c
	$(CC) -O2 -Wall -fPIC -shared -o $@ $<
clean:
	rm -f vivefilter.so
