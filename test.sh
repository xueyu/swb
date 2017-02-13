#!/bin/bash

#valgrind --leak-check=full ./swb -c 1 -n 1 http://192.168.1.17:8200/admin/queryOnline?__signature=1
./swb -c 1 -n 1 http://192.168.1.17:8200/admin/queryOnline?__signature=1
