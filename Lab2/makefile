CC = gcc
CFLAGS = -Wall -std=c17 -O0
TARGETS = main-static main-shared main-dynamic
LIBDIR = lib/
OBJDIR = obj/

.PHONY: all
all: $(TARGETS)

libcollatz_library.a: collatz_library.c
	$(CC) -c collatz_library.c -o $(OBJDIR)collatz_library-static.o
	ar rcs $(LIBDIR)libcollatz_library.a $(OBJDIR)collatz_library-static.o

libcollatz_library.so: collatz_library.c
	$(CC) -fPIC -c collatz_library.c -o $(OBJDIR)collatz_library-shared.o
	$(CC) -shared $(OBJDIR)collatz_library-shared.o -o $(LIBDIR)libcollatz_library.so

main-static: libcollatz_library.a main.c
	$(CC) $(CFLAGS) main.c $(LIBDIR)libcollatz_library.a -o main-static

main-shared: libcollatz_library.so main.c
	$(CC) main.c -lcollatz_library -L ./$(LIBDIR)  -Wl,-rpath,./$(LIBDIR) -o main-shared

main-dynamic: libcollatz_library.so main.c
	$(CC) $(CFLAGS) main_for_dynamic.c -D DYNAMIC=1 -o main-dynamic -L./$(LIBDIR) -lcollatz_library -Wl,-rpath,./$(LIBDIR)

.PHONY: clean
clean:
	rm $(OBJDIR)*.o $(LIBDIR)/*.so $(LIBDIR)/*.a $(TARGETS)