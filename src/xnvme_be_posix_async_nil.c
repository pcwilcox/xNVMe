// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_POSIX_ENABLED
#include <errno.h>
#include <xnvme_queue.h>
#include <xnvme_dev.h>
#include <libxnvme.h>

#define XNVME_BE_POSIX_ASYNC_NIL_CTX_DEPTH_MAX 29

struct xnvme_queue_nil {
	struct xnvme_queue_base base;

	struct xnvme_cmd_ctx *ctx[XNVME_BE_POSIX_ASYNC_NIL_CTX_DEPTH_MAX];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_queue_nil) == XNVME_BE_QUEUE_STATE_NBYTES,
	"Incorrect size"
)

int
_posix_nil_init(struct xnvme_queue *queue, int XNVME_UNUSED(opts))
{
	if (queue->base.capacity > XNVME_BE_POSIX_ASYNC_NIL_CTX_DEPTH_MAX) {
		XNVME_DEBUG("FAILED: requested more than async-nil supports");
		return -EINVAL;
	}

	return 0;
}

int
_posix_nil_term(struct xnvme_queue *XNVME_UNUSED(queue))
{
	return 0;
}

int
_posix_nil_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_queue_nil *queue = (void *)q;
	unsigned completed = 0;

	max = max ? max : queue->base.outstanding;
	max = max > queue->base.outstanding ? queue->base.outstanding : max;

	while (completed < max) {
		unsigned cur = queue->base.outstanding - completed - 1;
		struct xnvme_cmd_ctx *ctx;

		ctx = queue->ctx[cur];
		if (!ctx) {
			XNVME_DEBUG("-{[THIS SHOULD NOT HAPPEN]}-");

			++completed;
			queue->base.outstanding -= completed;
			return -EIO;
		}

		ctx->cpl.status.sc = 0;
		ctx->async.cb(ctx, ctx->async.cb_arg);
		queue->ctx[cur] = NULL;

		++completed;
	};

	queue->base.outstanding -= completed;
	return completed;
}

int
_posix_nil_wait(struct xnvme_queue *queue)
{
	int acc = 0;

	while (queue->base.outstanding) {
		struct timespec ts1 = {.tv_sec = 0, .tv_nsec = 1000};
		int err;

		err = _posix_nil_poke(queue, 0);
		if (err >= 0) {
			acc += err;
			continue;
		}

		switch (err) {
		case -EAGAIN:
		case -EBUSY:
			nanosleep(&ts1, NULL);
			continue;

		default:
			return err;
		}
	}

	return acc;
}

static inline int
_posix_nil_cmd_io(struct xnvme_cmd_ctx *ctx, void *XNVME_UNUSED(dbuf),
		  size_t XNVME_UNUSED(dbuf_nbytes), void *XNVME_UNUSED(mbuf),
		  size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_queue_nil *queue = (void *)ctx->async.queue;

	if (queue->base.outstanding == queue->base.capacity) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}

	queue->ctx[queue->base.outstanding++] = ctx;

	return 0;
}
#endif

struct xnvme_be_async g_xnvme_be_posix_async_nil = {
	.id = "nil",
#ifdef XNVME_BE_POSIX_ENABLED
	.cmd_io = _posix_nil_cmd_io,
	.poke = _posix_nil_poke,
	.wait = _posix_nil_wait,
	.init = _posix_nil_init,
	.term = _posix_nil_term,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
#endif
};
