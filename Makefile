monitor: mond.c
	gcc -g -pthread mond.c -o mond

clean:
	rm mond example temp temp2
