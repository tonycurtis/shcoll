all: build test

build: shcoll.a

shcoll.a: trees.o barrier.o broadcast.o reduction.o collect.o
	ar cr shcoll.a trees.o barrier.o broadcast.o reduction.o collect.o

trees.o: src/util/trees.c
	oshcc -c $< -o $@

barrier.o: src/barrier.c
	oshcc -c $< -o $@

broadcast.o: src/broadcast.c
	oshcc -c $< -o $@

reduction.o: src/reduction.c
	oshcc -c $< -o $@

collect.o: src/collect.c
	oshcc -c $< -o $@


test: barrier.test broadcast.test reduction.test collect.test

barrier.test: test/barrier_test.c shcoll.a
	oshcc $< shcoll.a -Isrc -o $@

broadcast.test: test/broadcast_test.c shcoll.a
	oshcc $< shcoll.a -Isrc -o $@

reduction.test: test/reduction_test.c shcoll.a
	oshcc $< shcoll.a -Isrc -o $@

collect.test: test/collect_test.c shcoll.a
	oshcc $< shcoll.a -Isrc -o $@

clean:
	rm -f *.o *.a *.test
