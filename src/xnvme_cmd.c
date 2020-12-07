// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_sgl.h>

/**
 * Calling this requires that opts at least has `XNVME_CMD_SGL_DATA`
 */
static inline void
xnvme_sgl_setup(struct xnvme_cmd_ctx *ctx, void *data, void *meta)
{
	struct xnvme_sgl *sgl = data;
	uint64_t phys;

	ctx->cmd.common.psdt = XNVME_SPEC_PSDT_SGL_MPTR_CONTIGUOUS;

	if (sgl->ndescr == 1) {
		ctx->cmd.common.dptr.sgl = sgl->descriptors[0];
	} else {
		xnvme_buf_vtophys(ctx->dev, sgl->descriptors, &phys);

		ctx->cmd.common.dptr.sgl.unkeyed.type = XNVME_SPEC_SGL_DESCR_TYPE_LAST_SEGMENT;
		ctx->cmd.common.dptr.sgl.unkeyed.len = sgl->ndescr * sizeof(struct xnvme_spec_sgl_descriptor);
		ctx->cmd.common.dptr.sgl.addr = phys;
	}

	if ((ctx->opts & XNVME_CMD_UPLD_SGLM) && meta) {
		sgl = meta;

		xnvme_buf_vtophys(ctx->dev, sgl->descriptors, &phys);

		ctx->cmd.common.psdt = XNVME_SPEC_PSDT_SGL_MPTR_SGL;

		if (sgl->ndescr == 1) {
			ctx->cmd.common.mptr = phys;
		} else {
			sgl->indirect->unkeyed.type = XNVME_SPEC_SGL_DESCR_TYPE_LAST_SEGMENT;
			sgl->indirect->unkeyed.len = sgl->ndescr * sizeof(struct xnvme_spec_sgl_descriptor);
			sgl->indirect->addr = phys;

			xnvme_buf_vtophys(ctx->dev, sgl->indirect, &phys);
			ctx->cmd.common.mptr = phys;
		}
	}
}

void
xnvme_cmd_ctx_pr(const struct xnvme_cmd_ctx *ctx, int XNVME_UNUSED(opts))
{
	printf("xnvme_cmd_ctx: ");

	if (!ctx) {
		printf("~\n");
		return;
	}

	printf("{cdw0: 0x%x, sc: 0x%x, sct: 0x%x}\n", ctx->cpl.cdw0,
	       ctx->cpl.status.sc, ctx->cpl.status.sct);
}

void
xnvme_cmd_ctx_clear(struct xnvme_cmd_ctx *ctx)
{
	memset(ctx, 0x0, sizeof(*ctx));
}

struct xnvme_cmd_ctx
xnvme_cmd_ctx_from_dev(struct xnvme_dev *dev)
{
	struct xnvme_cmd_ctx ctx = { .dev = dev, .opts = XNVME_CMD_SYNC };

	return ctx;
}

struct xnvme_cmd_ctx *
xnvme_cmd_ctx_from_queue(struct xnvme_queue *queue)
{
	return xnvme_queue_get_cmd_ctx(queue);
}

int
xnvme_cmd_pass(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
	       size_t mbuf_nbytes)
{
	const int cmd_opts = ctx->opts & XNVME_CMD_MASK;

	if ((cmd_opts & XNVME_CMD_MASK_UPLD) && dbuf) {
		xnvme_sgl_setup(ctx, dbuf, mbuf);
	}

	switch (cmd_opts & XNVME_CMD_MASK_IOMD) {
	case XNVME_CMD_ASYNC:
		return ctx->dev->be.async.cmd_io(ctx->dev, ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes, ctx->opts);

	case XNVME_CMD_SYNC:
		return ctx->dev->be.sync.cmd_io(ctx->dev, ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes,
						ctx->opts);

	default:
		XNVME_DEBUG("FAILED: command-mode not provided");
		return -EINVAL;
	}
}

int
xnvme_cmd_pass_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
		     size_t mbuf_nbytes)
{
	if (ctx->opts & XNVME_CMD_ASYNC) {
		XNVME_DEBUG("FAILED: Admin commands are always sync.");
		return -EINVAL;
	}

	if ((ctx->opts & XNVME_CMD_MASK_UPLD) && dbuf) {
		xnvme_sgl_setup(ctx, dbuf, mbuf);
	}

	return ctx->dev->be.sync.cmd_admin(ctx->dev, ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes, ctx->opts);
}
