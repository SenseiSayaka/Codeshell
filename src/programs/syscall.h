#pragma once
#define SYS_EXIT 1
#define SYS_WRITE 2
#define SYS_GETPID 3
static inline void sys_exit(int code){
	asm volatile(
			"int $0x80"
			:: "a"(SYS_EXIT), "b"(code)
		    );
}
static inline int sys_write(const char* str, unsigned int len){
	int ret;
	asm volatile(
			"int $0x80"
			: "=a"(ret)
			: "a"(SYS_WRITE), "b"(str), "c"(len)
			: "memory"
		    );
	return ret;
}
static inline int sys_getpid(){
	int ret;
	asm volatile(
			"int $0x80"
			: "=a"(ret)
			: "a"(SYS_GETPID)
		    );
	return ret;
}
static inline int sys_read(char* buf,unsigned int max){
	int ret;
	asm volatile(
			"int $0x80"
			: "=a"(ret)
			: "a"(4), "b"(buf), "c"(max)
			: "memory"
		    );
		    return ret;
}
static inline unsigned int sys_gettime(){
	unsigned int ret;
	asm volatile(
			"int $0x80"
			: "=a"(ret)
			: "a"(5)
		    );
	return ret;
}
static inline unsigned int mystrlen(const char* s){
	unsigned int n=0;
	while(s[n])n++;
	return n;
}
static inline void print(const char* s){
	sys_write(s, mystrlen(s));
}
