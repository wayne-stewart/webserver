

#include <stdio.h>


typedef enum AA {
	AA_ONE,
	AA_TWO
} AA;

int main(int argc, char** argv) {
	printf("hello, world %d %d\n", AA_ONE, AA_TWO);
}
