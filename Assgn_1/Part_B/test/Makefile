SRC = $(wildcard test_*.c)
EXE = $(SRC:.c=)

all: $(EXE)

%: %.c
	gcc -o $@ $< -lm -pthread -Wall -fdiagnostics-color -std=c17

clean:
	rm -f $(EXE)
