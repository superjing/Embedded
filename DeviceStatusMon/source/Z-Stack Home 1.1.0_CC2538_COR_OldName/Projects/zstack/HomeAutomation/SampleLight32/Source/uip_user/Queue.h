#ifndef QUEUE_H__
#define QUEUE_H__

#include <stdint.h>
#include <stdbool.h>

#define MAX_ELEMENT_NUM 32
#define ELEMENT_SIZE    21

typedef struct
{
   uint16_t front;
   uint16_t rear;
   //uint8_t elements[MAX_ELEMENT_NUM * ELEMENT_SIZE];
   uint8_t* elements;
}LockFreeQueue;

void InitFreeQueque( LockFreeQueue * pQueue );
bool IsFreeQueueFull( const LockFreeQueue * pQueue );
uint8_t QueueLength( const LockFreeQueue * pQueue );

void FreeQueuePop( LockFreeQueue * pQueue, uint8_t * pOutBuffer );
void FreeQueuePush( LockFreeQueue * pQueue, const uint8_t * elementBuffer );
#endif
