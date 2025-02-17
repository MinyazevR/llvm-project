#include <stdio.h>
int main(int argc, char** argv) {
	if(argc > 1) {
		printf("%d", 1);
	  	goto L1;
	  	L3:
	  	 return -1;
	} else {
		printf("%d", 10);
	}
	L1:
	  printf("%d", 2);
	  goto L3;
	return 0;
}
