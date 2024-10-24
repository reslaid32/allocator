#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MEMALLOCATE(T, S) (T*)malloc(S)
#define TYPEALLOCATE(T) MEMALLOCATE(T, sizeof(T))

typedef struct MemoryBlock {
    size_t szSize;
    bool isInited;
    bool isUsed;
    void* pAddr;
    struct MemoryBlock* pNext;
} MemoryBlock;

typedef struct Allocator {
    bool isInited;
    MemoryBlock* pHead;
} Allocator;

#define SUCCESS 1
#define ALREADY_INITED 2
#define INITIALIZATION_FAILURE 3
#define ALREADY_ALLOCATED 4
#define ALLOCATION_FAILURE 5

#define GETPROP(pSelf, _Property) (pSelf)->_Property
#define SETPROP(pSelf, _Property, _Value) (GETPROP(pSelf, _Property) = (_Value))

#define CHECKSELF(pSelf, _Operand, _Expected, _Trigger) \
    if ((pSelf) _Operand (_Expected)) _Trigger

#define CHECKPROP(pSelf, _Operand, _Property, _Expected, _Trigger) \
    if (GETPROP(pSelf, _Property) _Operand (_Expected)) _Trigger

int Allocator_Init(Allocator* pSelf) {
    CHECKPROP(pSelf, ==, isInited, true, return ALREADY_INITED);
    SETPROP(pSelf, isInited, true);
    SETPROP(pSelf, pHead, NULL);
    return SUCCESS;
}

void Allocator_InitBlock(MemoryBlock* pMemBlock, size_t szBlockSize, bool isBlockUsed, void* pBlockAddr, MemoryBlock* pNextMemBlock) {
    SETPROP(pMemBlock, szSize, szBlockSize);
    SETPROP(pMemBlock, isUsed, isBlockUsed);
    SETPROP(pMemBlock, pAddr, pBlockAddr);
    SETPROP(pMemBlock, pNext, pNextMemBlock);
    SETPROP(pMemBlock, isInited, true);
}

int Allocator_AllocateBlock(Allocator* pSelf, MemoryBlock** pMemBlock, size_t szSize) {
    CHECKSELF(*pMemBlock, !=, NULL, return ALREADY_ALLOCATED);
    void* pPointer = MEMALLOCATE(void, szSize);
    CHECKSELF(pPointer, ==, NULL, return ALLOCATION_FAILURE);
    *pMemBlock = TYPEALLOCATE(MemoryBlock);
    CHECKSELF(*pMemBlock, ==, NULL, return INITIALIZATION_FAILURE);
    Allocator_InitBlock(*pMemBlock, szSize, true, pPointer, GETPROP(pSelf, pHead));
    SETPROP(pSelf, pHead, *pMemBlock);
    return SUCCESS;
}

void Allocator_FreeBlock(MemoryBlock* pMemBlock) {
    if (pMemBlock && pMemBlock->isUsed) {
        free(pMemBlock->pAddr);
        pMemBlock->pAddr = NULL;
        pMemBlock->isUsed = false;
    }
}

void Allocator_Collect(Allocator* pSelf) {
    MemoryBlock* pCurrent = GETPROP(pSelf, pHead);
    MemoryBlock* pPrev = NULL;
    
    while (pCurrent != NULL) {
        if (!pCurrent->isUsed) {
            if (pPrev) {
                pPrev->pNext = pCurrent->pNext;
            } else {
                SETPROP(pSelf, pHead, pCurrent->pNext);
            }
            Allocator_FreeBlock(pCurrent);
            free(pCurrent);
            pCurrent = (pPrev) ? pPrev->pNext : GETPROP(pSelf, pHead);
        } else {
            pPrev = pCurrent;
            pCurrent = pCurrent->pNext;
        }
    }
}

void Allocator_Destroy(Allocator* pSelf) {
    MemoryBlock* pCurrent = GETPROP(pSelf, pHead);
    while (pCurrent != NULL) {
        MemoryBlock* pTemp = pCurrent;
        pCurrent = pCurrent->pNext;
        Allocator_FreeBlock(pTemp);
        free(pTemp);
    }
    free(pSelf);
}

void Allocator_WriteToBlock(MemoryBlock* pMemBlock, const void* pData, size_t szSize) {
    if (!pMemBlock->isUsed || szSize > pMemBlock->szSize) return;
    memcpy(pMemBlock->pAddr, pData, szSize);
}

void* Allocator_ReadFromBlock(MemoryBlock* pMemBlock) {
    if (!pMemBlock->isUsed) return NULL;
    return pMemBlock->pAddr;
}

void Test_Allocator_Init() {
    Allocator* allocator = TYPEALLOCATE(Allocator);
    int result = Allocator_Init(allocator);
    if (result == SUCCESS) {
        printf("Allocator initialized successfully.\n");
    } else {
        printf("Allocator initialization failed: %d\n", result);
    }
    Allocator_Destroy(allocator);
}

void Test_Allocator_AllocateBlock() {
    Allocator* allocator = TYPEALLOCATE(Allocator);
    Allocator_Init(allocator);
    MemoryBlock* block = NULL;
    int result = Allocator_AllocateBlock(allocator, &block, 1024);
    if (result == SUCCESS && block != NULL && block->isUsed) {
        printf("Block allocated successfully: Size = %zu\n", block->szSize);
    } else {
        printf("Block allocation failed: %d\n", result);
    }
    Allocator_FreeBlock(block);
    Allocator_Destroy(allocator);
}

void Test_Allocator_Collect() {
    Allocator* allocator = TYPEALLOCATE(Allocator);
    Allocator_Init(allocator);
    MemoryBlock* block1 = NULL;
    MemoryBlock* block2 = NULL;
    Allocator_AllocateBlock(allocator, &block1, 1024);
    Allocator_AllocateBlock(allocator, &block2, 2048);
    
    block1->isUsed = false;
    Allocator_Collect(allocator);
    
    if (allocator->pHead == block2) {
        printf("Garbage collection successful, block2 remains.\n");
    } else {
        printf("Garbage collection failed.\n");
    }
    
    Allocator_FreeBlock(block2);
    Allocator_Destroy(allocator);
}

void Test_Allocator_WriteReadBlock() {
    Allocator* allocator = TYPEALLOCATE(Allocator);
    Allocator_Init(allocator);
    MemoryBlock* block = NULL;
    Allocator_AllocateBlock(allocator, &block, 1024);

    const char* testData = "Hello, Allocator!";
    Allocator_WriteToBlock(block, testData, strlen(testData) + 1);

    char* readData = (char*)Allocator_ReadFromBlock(block);
    if (readData) {
        printf("Data read from block: %s\n", readData);
    } else {
        printf("Failed to read from block.\n");
    }

    const char* newData = "New data written!";
    Allocator_WriteToBlock(block, newData, strlen(newData) + 1);

    readData = (char*)Allocator_ReadFromBlock(block);
    if (readData) {
        printf("Data read from block after writing new data: %s\n", readData);
    } else {
        printf("Failed to read from block.\n");
    }

    Allocator_FreeBlock(block);
    Allocator_Destroy(allocator);
}

int main() {
    Test_Allocator_Init();
    Test_Allocator_AllocateBlock();
    Test_Allocator_Collect();
    Test_Allocator_WriteReadBlock();
    return 0;
}
