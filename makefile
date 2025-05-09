CC = gcc
CFLAGS = -Wall -O

all: emitter_test system_test cpu_test big_endian_test runtime_test \
	alloc_free_test tables_test source_test scanner_test n0 nx0 test_nx

emitter.o: emitter.c emitter.h opcode.h
	$(CC) $(CFLAGS) -c emitter.c

emitter_test: emitter_test.c emitter.h opcode.h \
		emitter.o
	$(CC) $(CFLAGS) emitter_test.c -o tmp_emitter_test \
		emitter.o
	./tmp_emitter_test

system.o: system.c system.h
	$(CC) $(CFLAGS) -c system.c

system_test: system_test.c system.h \
		system.o
	$(CC) $(CFLAGS) system_test.c -o tmp_system_test \
		system.o
	./tmp_system_test

cpu.o: cpu.c cpu.h opcode.h system.h
	$(CC) $(CFLAGS) -c cpu.c

cpu_test: cpu_test.c cpu.h emitter.h opcode.h \
		cpu.o system.o emitter.o
	$(CC) $(CFLAGS) cpu_test.c -o tmp_cpu_test \
		cpu.o system.o emitter.o
	./tmp_cpu_test

cpu_hello_world: cpu_hello_world.c cpu.h system.h emitter.h opcode.h \
		cpu.o system.o emitter.o
	$(CC) $(CFLAGS) cpu_hello_world.c -o tmp_cpu_hello_world \
		cpu.o system.o emitter.o
	./tmp_cpu_hello_world

big_endian.o: big_endian.c big_endian.h
	$(CC) $(CFLAGS) -c big_endian.c

big_endian_test: big_endian_test.c big_endian.h \
		big_endian.o
	$(CC) $(CFLAGS) big_endian_test.c -o tmp_big_endian_test \
		big_endian.o
	./tmp_big_endian_test

runtime.o: runtime.c runtime.h big_endian.h cpu.h opcode.h
	$(CC) $(CFLAGS) -c runtime.c

runtime_test: runtime_test.c runtime.h cpu.h emitter.h opcode.h \
		runtime.o big_endian.o cpu.o system.o emitter.o
	$(CC) $(CFLAGS) runtime_test.c -o tmp_runtime_test \
		runtime.o big_endian.o cpu.o system.o emitter.o
	./tmp_runtime_test

alloc_free.o: alloc_free.c alloc_free.h
	$(CC) $(CFLAGS) -c alloc_free.c

alloc_free_test: alloc_free_test.c \
		alloc_free.o
	$(CC) $(CFLAGS) alloc_free_test.c -o tmp_alloc_free_test \
		alloc_free.o
	./tmp_alloc_free_test

tables.o: tables.c tables.h clazz.h alloc_free.h
	$(CC) $(CFLAGS) -c tables.c

tables_test: tables_test.c \
		tables.o alloc_free.o
	$(CC) $(CFLAGS) tables_test.c -o tmp_tables_test \
		tables.o alloc_free.o
	./tmp_tables_test

source.o: source.c source.h
	$(CC) $(CFLAGS) $(LOGGER) -c source.c

source_test: source_test.c \
		source.o
	$(CC) $(CFLAGS) $(LOGGER) source_test.c -o tmp_source_test \
		source.o
	./tmp_source_test

scanner.o: scanner.c scanner.h source.h
	$(CC) $(CFLAGS) -c scanner.c

scanner_test: scanner_test.c scanner.h source.h \
		scanner.o source.o
	$(CC) $(CFLAGS) scanner_test.c -o tmp_scanner_test \
		scanner.o source.o
	./tmp_scanner_test

generator.o: generator.c generator.h clazz.h emitter.h opcode.h
	$(CC) $(CFLAGS) -c generator.c

parser.o: parser.c parser.h generator.h scanner.h source.h tables.h clazz.h \
		runtime.h cpu.h opcode.h
	$(CC) $(CFLAGS) -c parser.c

n0: n0.c parser.h runtime.h big_endian.h cpu.h system.h opcode.h source.h \
		parser.o generator.o scanner.o source.o tables.o alloc_free.o \
		runtime.o big_endian.o cpu.o system.o emitter.o
	$(CC) $(CFLAGS) n0.c -o n0 \
		parser.o generator.o scanner.o source.o tables.o alloc_free.o \
		runtime.o big_endian.o cpu.o system.o emitter.o

nx0: nx0.c runtime.h system.h \
		runtime.o big_endian.o cpu.o system.o
	$(CC) $(CFLAGS) nx0.c -o nx0 \
		runtime.o big_endian.o cpu.o system.o

test_nx: n0
	./n0 test01
	./n0 test05
	./n0 test07
	./n0 test11
	./n0 test13
	./n0 test17
	./n0 test19

clean:
	rm *.o tmp* n*0* test*.nx0
