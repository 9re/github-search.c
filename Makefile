github-search: parson.o github-search.o
	gcc -o github-search github-search.o parson.o -lcurl


parson.o: parson/parson.c
	gcc -c parson/parson.c -I./parson -o parson.o

github-search.o: github-search.c
	gcc -Wall -std=c99 -c github-search.c -I./parson


clean:
	rm *.o
