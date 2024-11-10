

#include <stdio.h>


typedef enum AA {
	AA_ONE,
	AA_TWO
} AA;

int main(int argc, char** argv) {
	printf("hello, world %d %d\n", AA_ONE, AA_TWO);

	float f1 = 10.311;
	double d1 = 10.311;

	printf("float: %.10f\n", f1);
	printf("double: %.10f\n", d1);
}
