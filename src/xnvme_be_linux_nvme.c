// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Gurmeet Singh <gur.singh@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifdef XNVME_BE_LINUX_ENABLED
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <linux/nvme_ioctl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_nvme.h>

#ifdef XNVME_DEBUG_ENABLED
static const char *
ioctl_request_to_str(unsigned long req)
{
	switch (req) {
	case NVME_IOCTL_ID:
		return "NVME_IOCTL_ID";
	case NVME_IOCTL_ADMIN_CMD:
		return "NVME_IOCTL_ADMIN_CMD";
	case NVME_IOCTL_SUBMIT_IO:
		return "NVME_IOCTL_SUBMIT_IO";
	case NVME_IOCTL_IO_CMD:
		return "NVME_IOCTL_IO_CMD";
	case NVME_IOCTL_RESET:
		return "NVME_IOCTL_RESET";
	case NVME_IOCTL_SUBSYS_RESET:
		return "NVME_IOCTL_SUBSYS_RESET";
	case NVME_IOCTL_RESCAN:
		return "NVME_IOCTL_RESCAN";

	default:
		return "NVME_IOCTL_UNKNOWN";
	}
}
#endif

static inline int
ioctl_wrap(struct xnvme_dev *dev, unsigned long ioctl_req, struct xnvme_cmd_ctx *ctx)
{
	struct xnvme_be_linux_state *state = (void *)dev->be.state;
	int err;
	
	err = ioctl(state->fd, ioctl_req, ctx);
	if (!err) {	// No errors
		return 0;
	}

	XNVME_DEBUG("FAILED: ioctl(%s), err(%d), errno(%d)", ioctl_request_to_str(ioctl_req),
		    err, errno);

	// Transform ioctl EINVAL to Invalid field in command
	if (err == -1 && errno == EINVAL) {
		XNVME_DEBUG("INFO: ioctl-errno(EINVAL) => INV_FIELD_IN_CMD");
		XNVME_DEBUG("INFO: overwrr. err(%d) with '0x2'", err);
		err = 0x2;
	}
	if (err > 0 && !ctx->cpl.status.val) {
		XNVME_DEBUG("INFO: overwr. cpl.status.val(0x%x) with '%d'",
			    ctx->cpl.status.val, err);
		ctx->cpl.status.val = err;
	}
	if (!errno) {
		XNVME_DEBUG("INFO: !errno, setting errno=EIO");
		errno = EIO;
	}

	return err;
}

int
xnvme_be_linux_nvme_cmd_io(struct xnvme_dev *dev, struct xnvme_cmd_ctx *ctx, void *dbuf,
			   size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes,
			   int XNVME_UNUSED(opts))
{
	int err;

	ctx->cmd.common.dptr.lnx_ioctl.data = (uint64_t)dbuf;
	ctx->cmd.common.dptr.lnx_ioctl.data_len = dbuf_nbytes;

	ctx->cmd.common.mptr = (uint64_t)mbuf;
	ctx->cmd.common.dptr.lnx_ioctl.metadata_len = mbuf_nbytes;

	err = ioctl_wrap(dev, NVME_IOCTL_IO_CMD, ctx);
	if (err) {
		XNVME_DEBUG("FAILED: ioctl_wrap(), err: %d", err);
		return err;
	}

	return 0;
}

int
xnvme_be_linux_nvme_cmd_admin(struct xnvme_dev *dev, struct xnvme_cmd_ctx *ctx, void *dbuf,
			      size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes,
			      int XNVME_UNUSED(opts))
{
	int err;

	ctx->cmd.common.dptr.lnx_ioctl.data = (uint64_t)dbuf;
	ctx->cmd.common.dptr.lnx_ioctl.data_len = dbuf_nbytes;

	ctx->cmd.common.mptr = (uint64_t)mbuf;
	ctx->cmd.common.dptr.lnx_ioctl.metadata_len = mbuf_nbytes;

	err = ioctl_wrap(dev, NVME_IOCTL_ADMIN_CMD, ctx);
	if (err) {
		XNVME_DEBUG("FAILED: ioctl_wrap() err: %d", err);
		return err;
	}

	return 0;
}

int
xnvme_be_linux_nvme_dev_nsid(struct xnvme_dev *dev)
{
	struct xnvme_be_linux_state *state = (void *)dev->be.state;

	return ioctl(state->fd, NVME_IOCTL_ID);
}

int
xnvme_be_linux_nvme_supported(struct xnvme_dev *dev, uint32_t XNVME_UNUSED(opts))
{
	int nsid = xnvme_be_linux_nvme_dev_nsid(dev);

	return nsid > 0;
}

struct xnvme_be_sync g_linux_nvme = {
	.cmd_io = xnvme_be_linux_nvme_cmd_io,
	.cmd_admin = xnvme_be_linux_nvme_cmd_admin,
	.id = "nvme_ioctl",
	.enabled = 1,
	.supported = xnvme_be_linux_nvme_supported,
};

#endif

