
.PHONY : swb

swb : main.c http_parser.c env.c event.c client.c
	gcc --std=gnu11 -O0 $^ -o $@


clean : 
	rm swb
