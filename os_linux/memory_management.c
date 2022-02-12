#include "memory_management.h"

// void split(struct block *bestFit, size_t size)
// {
//     struct block *new = (void *)((void *)bestFit + sizeof(struct block) + size);
//     new->blockSize = (bestFit->blockSize) - sizeof(struct block) - size;
//     new->free = 1;
//     new->next = bestFit->next;
//     //new->pre = splitBlock;
//     bestFit->blockSize = size;
//     bestFit->free = 0;
//     bestFit->next = new;
// }

/* First fit */
// t_block find_block(t_block *last, size_t size) {
//     t_block b = first_block;
//     while(b && !(b->free && b->size >= size)) {
//         *last = b;
//         b = b->next;
//     }
//     return b;
// }

void newBlock()
{
    blockList->blockSize = 5000 - sizeof(struct block);
    blockList->free = 1;
    blockList->next = NULL;
}

void print(struct block *ptr)
{
    long int i = 0;
    while (ptr)
    {
        printf("[PTR=%p]\t[STARTPTR=%p]\t[SIZE=%ld]\t[STATUS=%s]\t[NEXTPTR=%p]\t\n", ptr, ptr + 1, ptr->blockSize, (ptr->free == 0) ? "ALLOC" : "FREE", ptr->next);
        i += ptr->blockSize;
        ptr = ptr->next;
    }
    printf("[TOTAL=%ld]\n\n", i);
}

void *myMalloc(size_t size)
{
    // struct block *current,*previous;
    // void *result;
    // if(!(freeList->size))
    // {
    //     initialize();
    // }
    // current=freeList;
    // while((((current->size)<size)||((current->free)==0))&&(current->next!=NULL))
    // {
    //     previous=current;
    //     current=current->next;
    // }
    // if((current->size)==size)
    // {
    //     current->free=0;
    //     result=(void*)(++current);
    //     return result;
    // }
    // else if((current->size)>(size+sizeof(struct block)))            //所需要的内存大小小于区块大小
    // {
    //     split(current,size);                                        //分割区块函数
    //     result=(void*)(++current);                                       //使用的位置
    //     return result;
    // }
    // else
    // {
    //     result=NULL;
    //     return result;
    // }
    int diff_1 = 10000;
    int diff_0 = 0;
    int ignore_0 = 10000;
    //struct block *current, *previous;
    struct block *bestFit;
    struct block *current;
    current = blockList;
    void *returnPointer;
    if(ignore_0==10000)
    {
        ignore_0=100;
    }
    while (1)
    {
        while (current != NULL)
        {
            if (ignore_0)
            {
                if (current->blockSize == size)
                {
                    current->free = 0;
                    returnPointer = (void *)(++current);
                    return returnPointer;
                }
                else if (current->blockSize > size)
                {
                    diff_0 = current->blockSize - size;
                    if (diff_0 < diff_1)
                    {
                        diff_1 = diff_0;
                        bestFit = current;
                    }
                }
                //previous = current;
                current = current->next;
            }
        }
        break;
    }
    if (ignore_0)
    {
        while(1)
        {
            break;
        }
        if ((bestFit->blockSize) > (sizeof(struct block) + size))
        {
            struct block *new = (void *)((void *)bestFit + sizeof(struct block) + size);
            new->blockSize = (bestFit->blockSize) - sizeof(struct block) - size;
            new->free = 1;
            new->next = bestFit->next;
            //new->pre = splitBlock;
            bestFit->blockSize = size;
            bestFit->free = 0;
            bestFit->next = new;
            returnPointer = (void *)(++bestFit);
            ignore_0 = 100;
            return returnPointer;
        }
        else
        {
            ignore_0 = 1000;
            return NULL;
        }
    }
}

Block *merge(struct block *mergeBlock)
{
    int ignore_1 = 100;
    if (ignore_1)
    {
        if (mergeBlock->next && mergeBlock->next->free)
        {
            ignore_1 = 100;
            mergeBlock->blockSize += sizeof(struct block) + mergeBlock->next->blockSize;
            mergeBlock->next = mergeBlock->next->next;
        }
    }
    return mergeBlock;
}

void free(void *ptr)
{
    int ignore_2 = 10;
    if (ignore_2)
    {
        while(1)
        {
            break;
        }
        if (ptr >= (void *)memory && ptr <= (void *)(memory + 3000))
        {
            struct block *current = ptr;
            current--;
            current->free = 1;
            // while (current->pre && current->pre->free)
            // {
            //     current = merge(current->pre);
            // }

            while (current->next) // current will change
            {
                merge(current);
            }
            ignore_2 = 8;
        }
        else
        {
            return;
        }
    }
}

int main()
{
    newBlock();
    char *str = (char *)myMalloc(200);
    print(blockList);
    free(str);
    char *a = (char *)myMalloc(1000);
    print(blockList);
    char *b = (char *)myMalloc(2000);
    print(blockList);
    free(b);
    char *c = (char *)myMalloc(1500);
    print(blockList);
    // free(str);
    //free(blockList);

    // char *a = (char*)myMalloc(200);
    // print(blockList);
    // char *b = (char*)myMalloc(2000);
    // print(blockList);
    return 0;
}
