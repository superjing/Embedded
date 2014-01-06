#ifndef QUEUE_H__
#define QUEUE_H__

#include <stdint.h>
#include <stdbool.h>


#define MAX_ELEMENT_NUM      40
#define ELEMENT_SIZE         8
#define ELEMENT_NUM_TO_SEND  1

typedef struct
{
   uint8_t front;
   uint8_t rear;
   uint8_t elements[MAX_ELEMENT_NUM * ELEMENT_SIZE];
}LockFreeQueue;

void InitFreeQueque( LockFreeQueue * pQueue );
bool IsFreeQueueFull( const LockFreeQueue * pQueue );
uint8_t QueueLength( const LockFreeQueue * pQueue );

void FreeQueuePop( LockFreeQueue * pQueue, uint8_t * pOutBuffer );
void FreeQueuePush( LockFreeQueue * pQueue, const uint8_t * elementBuffer );
#endif
