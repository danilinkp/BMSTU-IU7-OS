CFLAGS := -pthread


app1: app1.o
	gcc -o $@  $^ $(CFLAGS)
	./$@


app1_threads: app1_threads.o
	gcc -o $@  $^ $(CFLAGS)
	./$@


app2: app2.o
	gcc -o $@  $^ $(CFLAGS)
	./$@


app2_threads: app2_threads_right.o
	gcc -o $@  $^ $(CFLAGS)


app3: app3.o
	gcc -o $@  $^ $(CFLAGS)

app3_threads: app3_threads.o
	gcc -o $@  $^ $(CFLAGS)

%.o: %.c
	gcc -c $^


clean:
	rm -f *.o src/*.o
