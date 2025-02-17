#include <stdio.h>
int main(int argc, char** argv) {
	if(argc > 1) {
		printf("%d", 1);
	  	if (argc > 5) {
	  		printf("%d", 6);
	  	} else {
	  		printf("%d", 5);
	    		if (argc > 10) {
	    			printf("%d", 10);
	      			return -1;
	    		}
	  	}
	} else {
		printf("%d", 6);
	}
	return 228;
}
