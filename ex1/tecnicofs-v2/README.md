Nota prévia:

O nosso projeto teve, durante algum tempo, um erro de compilação em
vários dispositivos e até no sigma sempre que tentávamos compilar com
as flags do fsanitize. O erro não permitia criar os executáveis dos
testes e era este:

cc -std=c11 -D_POSIX_C_SOURCE=200809L -fsanitize=thread -Ifs -I.
-fdiagnostics-color=always -Wall -Werror -Wextra -Wcast-align
-Wconversion -Wfloat-equal -Wformat=2 -Wnull-dereference -Wshadow
-Wsign-conversion -Wswitch-default -Wswitch-enum -Wundef
-Wunreachable-code -Wunused -pthread -Wno-sign-compare -g -O3   -c -o
tests/test1.o tests/test1.c
cc -std=c11 -D_POSIX_C_SOURCE=200809L -fsanitize=thread -Ifs -I.
-fdiagnostics-color=always -Wall -Werror -Wextra -Wcast-align
-Wconversion -Wfloat-equal -Wformat=2 -Wnull-dereference -Wshadow
-Wsign-conversion -Wswitch-default -Wswitch-enum -Wundef
-Wunreachable-code -Wunused -pthread -Wno-sign-compare -g -O3   -c -o
fs/operations.o fs/operations.c
cc -std=c11 -D_POSIX_C_SOURCE=200809L -fsanitize=thread -Ifs -I.
-fdiagnostics-color=always -Wall -Werror -Wextra -Wcast-align
-Wconversion -Wfloat-equal -Wformat=2 -Wnull-dereference -Wshadow
-Wsign-conversion -Wswitch-default -Wswitch-enum -Wundef
-Wunreachable-code -Wunused -pthread -Wno-sign-compare -g -O3   -c -o
fs/state.o fs/state.c
cc -pthread -fsanitize=thread  tests/test1.o fs/operations.o
fs/state.o   -o tests/test1
/usr/bin/ld: cannot find /usr/lib64/libtsan.so.0.0.0
collect2: error: ld returned 1 exit status
make: *** [<interno>: tests/test1] Erro 1

Por alguma razão conseguimos compilar com as flags recentemente mas
apenas num dos computadores.
