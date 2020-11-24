// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <libxnvme_nvm.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

int
xnvme_adm_format(struct xnvme_dev *dev, uint32_t nsid, uint8_t lbaf, uint8_t zf, uint8_t mset,
		 uint8_t ses, uint8_t pi, uint8_t pil, struct xnvme_cmd_ctx *ctx)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_FMT;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.format.lbaf = lbaf;
	ctx->cmd.format.zf = zf;
	ctx->cmd.format.mset = mset;
	ctx->cmd.format.pi = pi;
	ctx->cmd.format.pil = pil;
	ctx->cmd.format.ses = ses;

	return xnvme_cmd_pass_admin(dev, ctx, NULL, 0, NULL, 0, 0x0);
}

int
xnvme_nvm_sanitize(struct xnvme_dev *dev, uint8_t sanact, uint8_t ause, uint32_t ovrpat,
		   uint8_t owpass, uint8_t oipbp, uint8_t nodas, struct xnvme_cmd_ctx *ctx)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_SANITIZE;
	ctx->cmd.sanitize.sanact = sanact;
	ctx->cmd.sanitize.ause = ause;
	ctx->cmd.sanitize.owpass = owpass;
	ctx->cmd.sanitize.oipbp = oipbp;
	ctx->cmd.sanitize.nodas = nodas;
	ctx->cmd.sanitize.ause = ause;
	ctx->cmd.sanitize.ovrpat = ovrpat;

	return xnvme_cmd_pass_admin(dev, ctx, NULL, 0, NULL, 0, 0x0);
}

int
xnvme_nvm_read(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba, uint16_t nlb, void *dbuf,
	       void *mbuf, int opts, struct xnvme_cmd_ctx *ctx)
{
	size_t dbuf_nbytes = dbuf ? dev->geo.lba_nbytes * (nlb + 1) : 0;
	size_t mbuf_nbytes = mbuf ? dev->geo.nbytes_oob * (nlb + 1) : 0;

	// TODO: consider returning -EINVAL when mbuf is provided and namespace
	// have extended-lba in effect

	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.nvm.slba = slba;
	ctx->cmd.nvm.nlb = nlb;

	return xnvme_cmd_pass(dev, ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes, opts);
}

int
xnvme_nvm_write(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba, uint16_t nlb,
		const void *dbuf,
		const void *mbuf, int opts, struct xnvme_cmd_ctx *ctx)
{
	void *cdbuf = (void *)dbuf;
	void *cmbuf = (void *)mbuf;

	size_t dbuf_nbytes = cdbuf ? dev->geo.lba_nbytes * (nlb + 1) : 0;
	size_t mbuf_nbytes = cmbuf ? dev->geo.nbytes_oob * (nlb + 1) : 0;

	// TODO: consider returning -EINVAL when mbuf is provided and namespace
	// have extended-lba in effect

	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.nvm.slba = slba;
	ctx->cmd.nvm.nlb = nlb;

	return xnvme_cmd_pass(dev, ctx, cdbuf, dbuf_nbytes, cmbuf, mbuf_nbytes, opts);
}

int
xnvme_nvm_write_uncorrectable(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba, uint16_t nlb,
			      int opts, struct xnvme_cmd_ctx *ctx)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE_UNCORRECTABLE;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.nvm.slba = slba;
	ctx->cmd.nvm.nlb = nlb;

	return xnvme_cmd_pass(dev, ctx, NULL, 0, NULL, 0, opts);
}

int
xnvme_nvm_write_zeroes(struct xnvme_dev *dev, uint32_t nsid, uint64_t slba, uint16_t nlb, int opts,
		       struct xnvme_cmd_ctx *ctx)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE_ZEROES;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.write_zeroes.slba = slba;
	ctx->cmd.write_zeroes.nlb = nlb;

	return xnvme_cmd_pass(dev, ctx, NULL, 0, NULL, 0, opts);
}

int
xnvme_nvm_scopy(struct xnvme_dev *dev, uint32_t nsid, uint64_t sdlba,
		struct xnvme_spec_nvm_scopy_fmt_zero *ranges, uint8_t nr,
		enum xnvme_nvm_scopy_fmt copy_fmt, int opts, struct xnvme_cmd_ctx *ctx)
{
	size_t ranges_nbytes = 0;

	if (copy_fmt & XNVME_NVM_SCOPY_FMT_ZERO) {
		ranges_nbytes = (nr + 1) * sizeof(*ranges);
	}
	if (copy_fmt & XNVME_NVM_SCOPY_FMT_SRCLEN) {
		ranges_nbytes = (nr + 1) * sizeof(struct xnvme_spec_nvm_cmd_scopy_fmt_srclen);
	}

	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_SCOPY;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.scopy.sdlba = sdlba;
	ctx->cmd.scopy.nr = nr;

	// TODO: consider the remaining command-fields

	return xnvme_cmd_pass(dev, ctx, ranges, ranges_nbytes, NULL, 0, opts);
}

