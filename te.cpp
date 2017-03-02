#include <stdio.h>
#include <string.h>

char* array[2] = {"na", "op"};

int main(int argc,char **argv)
{
	if (argc < 2) {  
		printf("error!\n");
		printf("./main str\n");
		return -1;
	}
	
	if(!strcmp(argv[1], array[0])) printf("1\n");
	if(!strcmp(argv[1], array[1])) printf("2\n");
	
	return 0;
}
