#include <stm32.h>
#include "messages_queue.h"

// Clear the Message Queue:
// sets all the fields of the messages struct to 0
void clear_queue(messages_queue_t *queue)
{
    queue->read_position = 0;
    queue->insert_position = 0;
    queue->used_space = 0;
}

// Check if the Message Queue is empty:
// returns 1 if the queue is empty, 0 otherwise
uint8_t is_queue_empty(messages_queue_t *queue)
{
    return queue->used_space == 0;
}

// Check if the Message Queue is full:
// returns 1 if the queue is full, 0 otherwise
uint8_t is_queue_full(messages_queue_t *queue)
{
    return queue->used_space == MESSAGES_QUEUE_BUFFER_SIZE;
}

// Push a message to the Message Queue:
void enqueue(messages_queue_t *queue, char *message)
{
    queue->messages[queue->insert_position] = message;
    queue->insert_position = (queue->insert_position + 1) % MESSAGES_QUEUE_BUFFER_SIZE;
    queue->used_space++;
}

char *queue_poll(messages_queue_t *queue)
{
    char *message = queue->messages[queue->read_position];

    queue->read_position = (queue->read_position + 1) % MESSAGES_QUEUE_BUFFER_SIZE;
    queue->used_space--;

    return message;
}
