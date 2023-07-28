#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include <solros_ring_buffer.h>
#include <solros_ring_buffer_porting.h>
#include <solros_ring_buffer_i.h>

struct solros_ring_buffer_t *solros_ring_create(size_t size_hint, size_t align,
                       size_t socket_id, int blocking);

int solros_ring_enqueue(struct solros_ring_buffer_t *buf, void *data, size_t size,
           int blocking);

int solros_ring_dequeue(struct solros_ring_buffer_t *buf, void * data,
        size_t * size, int blocking);

void solros_ring_destroy(struct solros_ring_buffer_t *buf);

#endif
