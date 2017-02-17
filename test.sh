#!/bin/bash

#valgrind --leak-check=full ./swb -c 5 -n 100 http://192.168.1.17:8200/admin/queryOnline?__signature=1
./swb -c 4000 -n 10000 http://192.168.1.17:8200/admin/queryOnline?__signature=1
