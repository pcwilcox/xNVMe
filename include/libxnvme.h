/**
 * Cross-platform I/O library for NVMe devices
 *
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file libxnvme.h
 */
#ifndef __LIBXNVME_H
#define __LIBXNVME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <libxnvme_ident.h>
#include <libxnvme_be.h>
#include <libxnvme_spec.h>
#include <libxnvme_util.h>
#include <libxnvme_geo.h>
#include <libxnvme_dev.h>

/**
 * List of devices found on the system usable with xNVMe
 *
 * @struct xnvme_enumeration
 */
struct xnvme_enumeration {
	uint32_t capacity;		///< Remaining unused entries
	uint32_t nentries;		///< Used entries
	struct xnvme_ident entries[];	///< Device entries
};

/**
 * Enumerate devices on the given system
 *
 * @param list Pointer to pointer of the list of device enumerated
 * @param sys_uri URI of the system to enumerate on, when NULL, localhost/PCIe
 * @param opts System enumeration options
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_enumerate(struct xnvme_enumeration **list, const char *sys_uri, int opts);

/**
 * Opaque device handle.
 *
 * @see xnvme_dev_open()
 *
 * @struct xnvme_dev
 */
struct xnvme_dev;

/**
 * Creates a device handle (::xnvme_dev) based on the given device-uri
 *
 * @param dev_uri File path "/dev/nvme0n1" or "pci://0000:04.01?nsid=1"
 *
 * @return On success, a handle to the device. On error, NULL is returned and `errno` set to
 * indicate the error.
 */
struct xnvme_dev *
xnvme_dev_open(const char *dev_uri);

/**
 * Destroy the given device handle (::xnvme_dev)
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 */
void
xnvme_dev_close(struct xnvme_dev *dev);

/**
 * Allocate a buffer for IO with the given device
 *
 * The buffer will be aligned to device geometry and DMA allocated if required by the backend for
 * command payloads
 *
 * @note
 * nbytes must be greater than zero and a multiple of minimal granularity
 * @note
 * De-allocate the buffer using xnvme_buf_free()
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nbytes The size of the allocated buffer in bytes
 * @param phys A pointer to the variable to hold the physical address of the allocated buffer. If
 * NULL, the physical address is not returned.
 *
 * @return On success, a pointer to the allocated memory is returned. On error, NULL is returned
 * and `errno` set to indicate the error.
 */
void *
xnvme_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys);

/**
 * Reallocate a buffer for IO with the given device
 *
 * The buffer will be aligned to device geometry and DMA allocated if required by the backend for
 * IO
 *
 * @note
 * nbytes must be greater than zero and a multiple of minimal granularity
 * @note
 * De-allocate the buffer using xnvme_buf_free()
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param buf The buffer to reallocate
 * @param nbytes The size of the allocated buffer in bytes
 * @param phys A pointer to the variable to hold the physical address of the allocated buffer. If
 * NULL, the physical address is not returned.
 *
 * @return On success, a pointer to the allocated memory is returned. On error, NULL is returned
 * and `errno` set to indicate the error.
 */
void *
xnvme_buf_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes, uint64_t *phys);

/**
 * Free the given IO buffer allocated with xnvme_buf_alloc()
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param buf Pointer to a buffer allocated with xnvme_buf_alloc()
 */
void
xnvme_buf_free(const struct xnvme_dev *dev, void *buf);

/**
 * Retrieve the physical address of the given buffer
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param buf Pointer to a buffer allocated with xnvme_buf_alloc()
 * @param phys A pointer to the variable to hold the physical address of the given buffer.
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_buf_vtophys(const struct xnvme_dev *dev, void *buf, uint64_t *phys);

/**
 * Allocate a buffer of virtual memory of the given `alignment` and `nbytes`
 *
 * @note
 * You must use xnvme_buf_virt_free() to de-allocate the buffer
 *
 * @param alignment The alignment in bytes
 * @param nbytes The size of the buffer in bytes
 *
 * @return On success, a pointer to the allocated memory is return. On error, NULL is returned and
 * `errno` set to indicate the error
 */
void *
xnvme_buf_virt_alloc(size_t alignment, size_t nbytes);

/**
 * Free the given virtual memory buffer
 *
 * @param buf Pointer to a buffer allocated with xnvme_buf_virt_alloc()
 */
void
xnvme_buf_virt_free(void *buf);

/**
 * Opaque Command Queue Handle to be initialized by xnvme_queue_init()
 *
 * @see xnvme_queue_init
 * @see xnvme_queue_term
 *
 * @struct xnvme_queue
 */
struct xnvme_queue;

/**
 * Command Queue initialization options
 *
 * @enum xnvme_queue_opts
 */
enum xnvme_queue_opts {
	XNVME_QUEUE_IOPOLL = 0x1,       ///< XNVME_QUEUE_IOPOLL: queue. is polled for completions
	XNVME_QUEUE_SQPOLL = 0x1 << 1,  ///< XNVME_QUEUE_SQPOLL: queue. is polled for submissions
};

/**
 * Allocate a Command Queue for asynchronous command submission and completion
 *
 * @param dev Handle ::xnvme_dev obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param depth Maximum number of outstanding commands on the initialized queue, note that it must
 * be a power of 2 within the range [1,4096]
 * @param opts Queue options
 * @param queue Pointer-pointer to the ::xnvme_queue to initialize
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_queue_init(struct xnvme_dev *dev, uint16_t depth, int opts, struct xnvme_queue **queue);

/**
 * Get the I/O depth of the ::xnvme_queue
 *
 * @param queue Command context
 *
 * @return On success, depth of the given context is returned. On error, 0 is returned e.g. errors
 * are silent
 */
uint32_t
xnvme_queue_get_depth(struct xnvme_queue *queue);

/**
 * Get the number of outstanding commands on the given ::xnvme_queue
 *
 * @param queue Command Queue
 *
 * @return On success, number of outstanding commands are returned. On error, 0 is returned e.g.
 * errors are silent
 */
uint32_t
xnvme_queue_get_outstanding(struct xnvme_queue *queue);

/**
 * Tear down the given ::xnvme_queue
 *
 * @param queue
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_queue_term(struct xnvme_queue *queue);

/**
 * Process completions of commands on the given ::xnvme_queue
 *
 * Set process 'max' to limit number of completions, 0 means no max.
 *
 * @return On success, number of completions processed, may be 0. On error, negative `errno` is
 * returned.
 */
int
xnvme_queue_poke(struct xnvme_queue *queue, uint32_t max);

/**
 * Wait for completion of all outstanding commands in the given ::xnvme_queue
 *
 * @return On success, number of completions processed, may be 0. On error, negative `errno` is
 * returned.
 */
int
xnvme_queue_wait(struct xnvme_queue *queue);

/**
 * Retrieve a command-context from the given queue for async. command execution with the queue
 *
 * @note The command-context is managed by the queue, thus, return it to the queue via
 * ::xnvme_queue_put_cmd_ctx
 *
 * @note This is not thread-safe
 *
 * @param queue Pointer to the the ::xnvme_queue to retrieve a command-context for
 *
 * @return On success, a command-context is returned. On error, NULL is returned and `errno` is set
 * to indicate the error.
 */
struct xnvme_cmd_ctx *
xnvme_queue_get_cmd_ctx(struct xnvme_queue *queue);

/**
 * Hand back a command-context previously retrieve using ::xnvme_queue_get_cmd_ctx
 *
 * @note This function is not thread-safe
 *
 * @param queue Pointer to the ::xnvme_queue to hand back the command-context to
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_queue_put_cmd_ctx(struct xnvme_queue *queue, struct xnvme_cmd_ctx *ctx);

/**
 * Signature of function used with Command Queues for async. callback upon command-completion
 */
typedef void (*xnvme_queue_cb)(struct xnvme_cmd_ctx *ctx, void *opaque);

/**
 * The xNVMe Command Context
 *
 * @struct xnvme_cmd_ctx
 */
struct xnvme_cmd_ctx {
	struct xnvme_spec_cmd cmd;		///< Command to be processed
	struct xnvme_spec_cpl cpl;		///< Completion result from processing

	struct xnvme_dev *dev;

	///< Fields for command option: XNVME_CMD_ASYNC
	struct {
		struct xnvme_queue *queue;    ///< Queue used for command processing
		xnvme_queue_cb cb;        ///< User defined callback function
		void *cb_arg;            ///< User defined callback function arguments
	} async;

	uint32_t opts;

	uint8_t be_rsvd[4];

	///< Fields for cmd_ctx-pool
	SLIST_ENTRY(xnvme_cmd_ctx) link;
};

/**
 * Assign a callback-function and argument to be used with the ::xnvme_cmd_ctx of the queue
 *
 * @param queue The ::xnvme_queue to assign default callback function for
 * @param cb The callback function to use
 * @param cb_arg The callback argument to use
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_queue_set_cb(struct xnvme_queue *queue, xnvme_queue_cb cb, void *cb_arg);

/**
 * Enumeration of `xnvme_cmd` options
 *
 * @enum xnvme_cmd_opts
 */
enum xnvme_cmd_opts {
	XNVME_CMD_SYNC          = 0x1 << 0,     ///< XNVME_CMD_SYNC: Synchronous command
	XNVME_CMD_ASYNC         = 0x1 << 1,     ///< XNVME_CMD_ASYNC: Asynchronous command

	XNVME_CMD_UPLD_SGLD     = 0x1 << 2,     ///< XNVME_CMD_UPLD_SGLD: User-managed SGL data
	XNVME_CMD_UPLD_SGLM     = 0x1 << 3,     ///< XNVME_CMD_UPLD_SGLM: User-managed SGL meta
};

#define XNVME_CMD_MASK_IOMD ( XNVME_CMD_SYNC | XNVME_CMD_ASYNC )
#define XNVME_CMD_MASK_UPLD ( XNVME_CMD_UPLD_SGLD | XNVME_CMD_UPLD_SGLM )
#define XNVME_CMD_MASK ( XNVME_CMD_MASK_IOMD | XNVME_CMD_MASK_UPLD )

#define XNVME_CMD_DEF_IOMD XNVME_CMD_SYNC
#define XNVME_CMD_DEF_UPLD ( 0x0 )

/**
 * Pass a NVMe IO Command through to the device via the given ::xnvme_cmd_ctx
 *
 * When constructing the command then take note of the following:
 *
 * @note User-defined payloads -- When 'opts' include XNVME_CMD_UPLD_SGLD or XNVME_CMD_UPLD_SGLM
 * then the respective buffer points to a user-defined payload setup with the `xnvme_sgl` helper
 * functions.  cmd.pdst, data, and meta fields must also be setup by the caller.  `xnvme_sgl`
 * helper functions, pdst, data, and meta fields must be also be set by you
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param dbuf pointer to data-payload
 * @param dbuf_nbytes size of data-payload in bytes
 * @param mbuf pointer to meta-payload
 * @param mbuf_nbytes size of the meta-payload in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_pass(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
	       size_t mbuf_nbytes);

/**
 * Pass a NVMe Admin Command through to the device with minimal intervention
 *
 * @note User-defined payloads -- When 'opts' include XNVME_CMD_UPLD_SGLD or XNVME_CMD_UPLD_SGLM
 * then the respective buffer points to a user-defined payload setup with the `xnvme_sgl` helper
 * functions.  cmd.pdst, data, and meta fields must also be setup by the caller.  `xnvme_sgl`
 * helper functions, pdst, data, and meta fields must be also be set by you
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param dbuf pointer to data-payload
 * @param dbuf_nbytes size of data-payload in bytes
 * @param mbuf pointer to meta-payload
 * @param mbuf_nbytes size of the meta-payload in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_cmd_pass_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
		     size_t mbuf_nbytes);

/**
 * Retrieve a command-context for issuing commands to the given device
 *
 */
struct xnvme_cmd_ctx
xnvme_cmd_ctx_from_dev(struct xnvme_dev *dev);

/**
 * Retrieve a command-text for issuing commands to the given queue
 */
struct xnvme_cmd_ctx *
xnvme_cmd_ctx_from_queue(struct xnvme_queue *queue);

/**
 * Clears/resets the given ::xnvme_cmd_ctx
 *
 * @param ctx Pointer to the ::xnvme_cmd_ctx to clear
 */
void
xnvme_cmd_ctx_clear(struct xnvme_cmd_ctx *ctx);

/**
 * Encapsulate completion-error checking here for now.
 *
 * @todo re-think this
 * @param ctx Pointer to the ::xnvme_cmd_ctx to check status on
 *
 * @return On success, 0 is return. On error, a non-zero value is returned.
 */
static inline int
xnvme_cmd_ctx_cpl_status(struct xnvme_cmd_ctx *ctx)
{
	return ctx->cpl.status.sc || ctx->cpl.status.sct;
}

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_H */
