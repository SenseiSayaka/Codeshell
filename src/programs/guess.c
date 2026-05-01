#include "syscall.h"
//very simple generator random numbers
static char input[64];
static unsigned int seed=42;
static int random(int max){
	seed=seed*1103515245+12345;
	return (seed>>16)%max;
}
//strcmp for compare "exit"
static int streq(const char* a,const char* b){
	while(*a&&*b){if(*a!=*b)return 0;a++;b++;}
	return *a==*b;
}
//convert string to number
static int parse_int(const char* s){
	int n=0;
	while(*s>='0'&&*s<='9'){
		n=n*10+(*s-'0');
		s++;
	}return n;
}
//print number
static void print_int(int n){
	char buf[16];
	int i=0;if(n==0){print("0");return;}
	while(n>0){buf[i++]='0'+(n%10);n/=10;}
	while(--i>=0){
		char c[2]={buf[i],0};
		sys_write(c,1);
	}
}
int main(){
	seed=sys_gettime();
	int target=random(100)+1;
	char input[64];
	print("Guess the number 1-100!\n");
	print("(type 'exit' to quit)\n\n");
	while(1){
		print("> ");
		int n=sys_read(input,64);
		if(n<=0)continue;;
        print(input);
        print("' len=");
        print_int(n);
        print("\n");
		if(streq(input,"exit")){
			print("Bye!\n");
			return 0;
		}
		int guess=parse_int(input);
		if(guess==target){
			print("Correct!\n");
			return 0;
		}else if(guess<target){
			print("Higher!\n");
		}else{
			print("Lower!\n");
		}
	}
}
