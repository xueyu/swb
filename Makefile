
.PHONY : swb

swb : main.c http_parser.c env.c event.c client.c
	gcc --std=gnu11 $^ -o $@


clean : 
	rm swb
