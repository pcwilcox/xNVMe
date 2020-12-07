// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_queue.h>

int
xnvme_queue_term(struct xnvme_queue *queue)
{
	int err;

	if (!queue) {
		XNVME_DEBUG("FAILED: !dev");
		return -EINVAL;
	}

	err = queue->base.dev ? queue->base.dev->be.async.term(queue) : 0;
	if (err) {
		XNVME_DEBUG("FAILED: backend queue-termination failed with err: %d", err);
	}

	free(queue);

	return err;
}

int
xnvme_queue_init(struct xnvme_dev *dev, uint16_t depth, int opts, struct xnvme_queue **queue)
{
	size_t queue_nbytes;
	int err;

	if (!dev) {
		XNVME_DEBUG("FAILED: !dev");
		return -EINVAL;
	}
	if (!(xnvme_is_pow2(depth) && (depth < 4096))) {
		XNVME_DEBUG("EINVAL: depth: %u", depth);
		return -EINVAL;
	}

	queue_nbytes = sizeof(**queue) + (depth + 1) * sizeof(*((*queue)->pool_storage));

	*queue = calloc(1, queue_nbytes);
	if (!*queue) {
		XNVME_DEBUG("FAILED: calloc(queue), err: %s", strerror(errno));
		return -errno;
	}
	(*queue)->base.depth = depth;
	(*queue)->base.dev = dev;

	SLIST_INIT(&(*queue)->base.pool);

	for (uint32_t i = 0; i <= (*queue)->base.depth; ++i) {
		(*queue)->pool_storage[i].dev = dev;
		(*queue)->pool_storage[i].async.queue = *queue;
		(*queue)->pool_storage[i].async.cb = NULL;
		(*queue)->pool_storage[i].async.cb_arg = NULL;

		SLIST_INSERT_HEAD(&(*queue)->base.pool, &((*queue)->pool_storage[i]), link);
	}

	err = dev->be.async.init(*queue, opts);
	if (err) {
		XNVME_DEBUG("FAILED: backend-queue initialization with err: %d", err);
		free(*queue);
		return err;
	}

	return 0;
}

int
xnvme_queue_poke(struct xnvme_queue *queue, uint32_t max)
{
	if (!queue->base.outstanding) {
		return 0;
	}

	return queue->base.dev->be.async.poke(queue, max);
}

int
xnvme_queue_wait(struct xnvme_queue *queue)
{
	return queue->base.dev->be.async.wait(queue);
}

uint32_t
xnvme_queue_get_depth(struct xnvme_queue *queue)
{
	return queue->base.depth;
}

uint32_t
xnvme_queue_get_outstanding(struct xnvme_queue *queue)
{
	return queue->base.outstanding;
}

struct xnvme_cmd_ctx *
xnvme_queue_get_cmd_ctx(struct xnvme_queue *queue)
{
	struct xnvme_cmd_ctx *ctx = SLIST_FIRST(&queue->base.pool);

	if (!ctx) {
		errno = ENOMEM;
		return ctx;
	}

	SLIST_REMOVE_HEAD(&queue->base.pool, link);

	return ctx;
}

int
xnvme_queue_put_cmd_ctx(struct xnvme_queue *queue, struct xnvme_cmd_ctx *ctx)
{
	SLIST_INSERT_HEAD(&queue->base.pool, ctx, link);

	return 0;
}
