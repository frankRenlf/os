#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

char memory[5000];

// int brk(void *addr);
// void *sbrk(intptr_t increment);

// typedef struct s_block *t_block;
// struct s_block {
//     size_t size;  /* 数据区大小 */
//     t_block next; /* 指向下个块的指针 */
//     int free;     /* 是否是空闲块 */
//     int padding;  /* 填充4字节，保证meta块长度为8的倍数 */
//     char data[1]  /* 这是一个虚拟字段，表示数据块的第一个字节，长度不应计入meta */
// };

typedef struct block
{
    size_t blockSize; //区块大小
    int free;
    struct block *next;
    struct block *pre;
} Block;

void *myMalloc(size_t size);

// void split(struct block *splitBlock, size_t size);

struct block *blockList = (void *)memory;

Block *merge(struct block *mergeBlock);

void print(struct block *ptr);

void newBlock();