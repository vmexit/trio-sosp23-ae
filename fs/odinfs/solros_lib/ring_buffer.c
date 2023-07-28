#include <solros_ring_buffer.h>
#include <solros_ring_buffer_porting.h>
#include <solros_ring_buffer_i.h>
#include <linux/ratelimit.h>
#include <solros_ring_buffer_api.h>

struct solros_ring_buffer_t *solros_ring_create(size_t size_hint, size_t align,
					   size_t socket_id, int blocking)
{
	int rc;
	struct solros_ring_buffer_t *rb;

	rc = solros_ring_buffer_create(
		NULL, NULL, NULL, size_hint, align, socket_id, 0,
		(blocking ? SOLROS_RING_BUFFER_BLOCKING :
				  SOLROS_RING_BUFFER_NON_BLOCKING),
		NULL, NULL, &rb);

	if (rc != 0)
		printk(KERN_ERR "[%s:%d]: Failed to create ring buffer rc=%d",
		       __FUNCTION__, __LINE__, rc);

	return rb;
}

#ifdef RING_BUFFER_CONF_KERNEL
EXPORT_SYMBOL(solros_ring_create);
#endif

int solros_ring_enqueue(struct solros_ring_buffer_t *buf, void *data, size_t size,
		   int blocking)
{
	int rc;
	struct solros_ring_buffer_req_t solros_request;

	if (blocking)
		solros_ring_buffer_put_req_init(&solros_request, BLOCKING,
						size);
	else
		solros_ring_buffer_put_req_init(&solros_request, NON_BLOCKING,
						size);

	rc = solros_ring_buffer_put(buf, &solros_request);
	if (rc != 0) {

	    if (rc != -EAGAIN)
	    {
            printk_ratelimited(KERN_ERR
                       "[%s:%d]: Failed to enqueue request rc=%d",
                       __FUNCTION__, __LINE__, rc);
	    }

		goto done;
	}

	rc = solros_copy_to_ring_buffer(buf, solros_request.data, data,
					solros_request.size);

	if (rc != 0) {
		printk_ratelimited(
			KERN_ERR
			"[%s:%d]: Failed to copy request to ring buffer rc=%d",
			__FUNCTION__, __LINE__, rc);
		goto done;
	}

	solros_ring_buffer_elm_set_ready(buf, solros_request.data);

done:
	return rc;
}

#ifdef RING_BUFFER_CONF_KERNEL
EXPORT_SYMBOL(solros_ring_enqueue);
#endif

int solros_ring_dequeue(struct solros_ring_buffer_t *buf, void * data,
        size_t * size, int blocking)
{
	int rc;
	struct solros_ring_buffer_req_t solros_reply;

	if (blocking)
		solros_ring_buffer_get_req_init(&solros_reply, BLOCKING);
	else
		solros_ring_buffer_get_req_init(&solros_reply, NON_BLOCKING);

	rc = solros_ring_buffer_get(buf, &solros_reply);

	if (rc != 0)
	{
        if (rc != -EAGAIN)
        {
            printk_ratelimited(KERN_ERR
                       "[%s:%d]: Failed to dequeue request rc=%d",
                       __FUNCTION__, __LINE__, rc);
        }
		goto error;
	}

	if (data)
	{
        rc = solros_copy_from_ring_buffer(buf, data, solros_reply.data,
                solros_reply.size);

        if (rc != 0) {
            printk_ratelimited(
                KERN_ERR
                "[%s:%d]: Failed to copy from ring buffer rc=%d",
                __FUNCTION__, __LINE__, rc);
            goto error;
        }
	}


	if (size)
	    (*size) = solros_reply.size;

	solros_ring_buffer_elm_set_done(buf, solros_reply.data);

	return 0;

error:
	return rc;
}

#ifdef RING_BUFFER_CONF_KERNEL
EXPORT_SYMBOL(solros_ring_dequeue);
#endif

void solros_ring_destroy(struct solros_ring_buffer_t *buf)
{
	solros_ring_buffer_destroy(buf, 0, NULL);
}

#ifdef RING_BUFFER_CONF_KERNEL
EXPORT_SYMBOL(solros_ring_destroy);
#endif
