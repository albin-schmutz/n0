/*

Provides big endian converters for 32 and 64 bit integers.

*/

extern int be_to_i32(unsigned char b[]);
extern int64_t be_to_i64(unsigned char b[]);
extern void be_from_i32(unsigned char b[], int32_t i);
extern void be_from_i64(unsigned char b[], int64_t i);
