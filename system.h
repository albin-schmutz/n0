/*

Implements SYS opcode calls.
- Provides system infos.
- Can execute OS commands.
- Provides command line arguments.
- Provides environment variables.
- Provides file operations.

*/

#define SYS_INFO_OS 1
#define SYS_INFO_IO_STDIN 2
#define SYS_INFO_IO_STDOUT 3
#define SYS_INFO_IO_STDERR 4
#define SYS_INFO_ARGC 5

#define SYS_MODE_READ 1
#define SYS_MODE_WRITE 2

#define SYS_CALL_HALT 0
#define SYS_CALL_EXEC 1
#define SYS_CALL_GET_INFO 2
#define SYS_CALL_GET_ARG 3
#define SYS_CALL_GET_ENV 4
#define SYS_CALL_FILE_OPEN 5
#define SYS_CALL_FILE_CLOSE 6
#define SYS_CALL_FILE_READ 7
#define SYS_CALL_FILE_WRITE 8

#define BUFFER_SIZE 104

typedef struct BUFFER *BUFFER;

struct BUFFER {
	char data[BUFFER_SIZE];
	int64_t val[2];
	int32_t pos;
	int32_t limit;
};

extern void sys_init(int argc, char *argv[]);
extern void sys_call(int nr, int64_t *result, void *data);
