#include "Queue.h"

#include <string.h>
#include <assert.h>

void InitFreeQueque( LockFreeQueue * pQueue)
{
   pQueue->front = 0;
   pQueue->rear = 0;
   memset(pQueue->elements, 0xFF, MAX_ELEMENT_NUM * ELEMENT_SIZE );
}

bool IsFreeQueueFull( const LockFreeQueue * pQueue )
{
   assert(pQueue != NULL);

   if( (pQueue->rear + 1) == MAX_ELEMENT_NUM )
   {
      return ( 0 == pQueue->front );
   }
   else
   { 
      return ( ( pQueue->rear + 1 ) ==  pQueue->front );
   }
}

void FreeQueuePush( LockFreeQueue * pQueue, const uint8_t * elementBuffer )
{
   memcpy(
      pQueue->elements + pQueue->rear * ELEMENT_SIZE, 
      elementBuffer, 
      ELEMENT_SIZE
      );

   ++pQueue->rear;
      
   if( pQueue->rear == MAX_ELEMENT_NUM )
   {
      pQueue->rear = 0;
   }
}

uint8_t QueueLength( const LockFreeQueue * pQueue )
{
   if ( pQueue->rear >= pQueue->front)
   {
      return (pQueue->rear - pQueue->front);
   }
   else
   {
      return (pQueue->rear + MAX_ELEMENT_NUM - pQueue->front);
   }
}

void FreeQueuePop( LockFreeQueue * pQueue, uint8_t * pOutBuffer )
{
   memcpy(
      pOutBuffer, 
      pQueue->elements + pQueue->front * ELEMENT_SIZE, 
      ELEMENT_NUM_TO_SEND * ELEMENT_SIZE
      );

   pQueue->front += ELEMENT_NUM_TO_SEND;

   if( pQueue->front == MAX_ELEMENT_NUM )
   {
      pQueue->front = 0;
   }
}