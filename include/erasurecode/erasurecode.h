/* 
 * <Copyright>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.  THIS SOFTWARE IS PROVIDED BY
 * THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * liberasurecode frontend API header
 *
 * vi: set noai tw=79 ts=4 sw=4:
 */

#ifndef _ERASURECODE_H_
#define _ERASURECODE_H_

#include "list.h"
#include "erasurecode_stdinc.h"
#include "erasurecode_version.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =~=*=~==~=*=~==~=*=~= Supported EC backends =~=*=~==~=*=~==~=*=~==~=*=~== */

typedef enum {
    EC_BACKEND_NULL                     = 0,
    EC_BACKEND_JERASURE_RS_VAND         = 1,
    EC_BACKEND_JERASURE_RS_CAUCHY       = 2,
    EC_BACKEND_FLAT_XOR_HD              = 3,
    EC_BACKENDS_MAX,
} ec_backend_id_t;

#ifndef EC_BACKENDS_SUPPORTED
#define EC_BACKENDS_SUPPORTED
/* Supported EC backends */
static const char *ec_backend_names[EC_BACKENDS_MAX] = {
    "null",
    "jerasure_rs_vand",
    "jerasure_rs_cauchy",
    "flat_xor_hd",
};
#endif // EC_BACKENDS_SUPPORTED

/* =~=*=~==~=*=~== EC Arguments - Common and backend-specific =~=*=~==~=*=~== */

/**
 * Common and backend-specific args
 * to be passed to liberasurecode_instance_create()
 */
struct ec_args {
    int k;                  /* number of data fragments */
    int m;                  /* number of parity fragments */
    int w;                  /* word size, in bits (optional) */

    union {
        struct {
            int hd;         /* hamming distance (3 or 4) */
        } flat_xor_hd_args; /* args specific to XOR codes */
        struct {
            uint64_t arg1;  /* sample arg */
        } null_args;        /* args specific to the null codes */
        struct {
            uint64_t x, y;  /* reserved for future expansion */
            uint64_t z, a;  /* reserved for future expansion */
        } reserved;
    } priv_args1;

    void *priv_args2;       /** flexible placeholder for
                              * future backend args */

    int inline_chksum;      /* embedded fragment checksums (yes/no), type */
    int algsig_chksum;      /* use algorithmic signature checksums */
};

/* =~=*=~==~=*=~== liberasurecode frontend API functions =~=*=~==~=~=*=~==~= */

/* liberasurecode frontend API functions */

/**
 * Returns a list of EC backends implemented/enabled - the user
 * should always rely on the return from this function as this
 * set of backends can be different from the names listed in
 * ec_backend_names above.
 *
 * @param num_backends - pointer to return number of backends in
 *
 * @returns 
 */
const char ** liberasurecode_supported_backends(int *num_backends);

/**
 * Create a liberasurecode instance and return a descriptor 
 * for use with EC operations (encode, decode, reconstruct)
 *
 * @param backend_name - one of the supported backends as
 *        defined by ec_backend_names
 * @param ec_args - arguments to the EC backend
 *        arguments common to all backends
 *          k - number of data fragments
 *          m - number of parity fragments
 *          inline_checksum - 
 *          algsig_checksum -
 *        backend-specific arguments
 *          null_args - arguments for the null backend
 *          flat_xor_hd_args - arguments for the xor_hd backend
 *          jerasure_args - arguments for the Jerasure backend
 *      
 * @returns liberasurecode instance descriptor (int > 0)
 */
int liberasurecode_instance_create(const char *backend_name,
                                   struct ec_args *args);

/**
 * Close a liberasurecode instance
 *
 * @param liberasurecode descriptor to close
 */
int liberasurecode_instance_destroy(int desc);


/**
 * Erasure encode a data buffer
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param orig_data - data to encode
 * @param orig_data_size - length of data to encode
 * @param encoded_data - pointer to _output_ array (char **) of k data
 *        fragments (char *), allocated by the callee
 * @param encoded_parity - pointer to _output_ array (char **) of m parity
 *        fragments (char *), allocated by the callee
 * @param fragment_len - pointer to _output_ length of each fragment, assuming
 *        all fragments are the same length
 *
 * @return 0 on success, -error code otherwise
 */
int liberasurecode_encode(int desc,
        const char *orig_data, uint64_t orig_data_size, /* input */
        char ***encoded_data, char ***encoded_parity,   /* output */
        uint64_t *fragment_len);                        /* output */

/**
 * Reconstruct original data from a set of k encoded fragments
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param fragments - erasure encoded fragments (> = k)
 * @param num_fragments - number of fragments being passed in
 * @param fragment_len - length of each fragment (assume they are the same)
 * @param out_data - _output_ pointer to decoded data
 * @param out_data_len - _output_ length of decoded output
 *          (both output data pointers are allocated by liberasurecode,
 *           caller invokes liberasurecode_decode_clean() after it has
 *           read decoded data in 'out_data')
 *
 * @return 0 on success, -error code otherwise
 */
int liberasurecode_decode(int desc,
        char **available_fragments,                     /* input */
        int num_fragments, uint64_t fragment_len,       /* input */
        char **out_data, uint64_t *out_data_len);        /* output */

/**
 * Reconstruct a missing fragment from a subset of available fragments
 *
 * @param desc - liberasurecode descriptor/handle
 *        from liberasurecode_instance_create()
 * @param fragment_len - size in bytes of the fragments
 * @param available_fragments - erasure encoded fragments
 * @param num_fragments - number of fragments being passed in
 * @param destination_idx - missing idx to reconstruct
 * @param out_fragment - output of reconstruct
 *
 * @return 0 on success, -error code otherwise
 */
int liberasurecode_reconstruct_fragment(int desc,
        char **available_fragments,                     /* input */
        int num_fragments, uint64_t fragment_len,       /* input */
        int destination_idx,                            /* input */
        char* out_fragment);                            /* output */

/**
 * Determine which fragments are needed to reconstruct some subset
 * of missing fragments.  Returns a list of lists (as bitmaps)
 * of fragments required to reconstruct missing indexes.
 */
int liberasurecode_fragments_needed(int desc,
        int *missing_idxs, int *fragments_needed);

/**
 * Get opaque metadata for a fragment.  The metadata is opaque to the
 * client, but meaningful to the underlying library.  It is used to verify
 * stripes in verify_stripe_metadata().
 */
int liberasurecode_get_fragment_metadata(char *fragment);


/**
 * Verify a subset of fragments generated by encode()
 */
int liberasurecode_verify_stripe_metadata(char **fragments);

/**
 * This computes the aligned size of a buffer passed into 
 * the encode function.  The encode function must pad fragments
 * to be algined with the word size (w) and the last fragment also
 * needs to be aligned.  This computes the sum of the algined fragment
 * sizes for a given buffer to encode.
 */
int liberasurecode_get_aligned_data_size(int desc, uint64_t data_len);
 
/**
 * This will return the minumum encode size, which is the minimum
 * buffer size that can be encoded.
 */
int liberasurecode_get_minimum_encode_size(int desc);


/* ==~=*=~===~=*=~==~=*=~== liberasurecode Error codes =~=*=~==~=~=*=~==~== */

/* Error codes */
typedef enum {
    EBACKENDNOTSUPP  = 200,
    EECMETHODNOTIMPL = 201,
    EBACKENDINITERR  = 202,
    EBACKENDINUSE    = 203,
    EBACKENDNOTAVAIL = 204,
} LIBERASURECODE_ERROR_CODES;

/* =~=*=~==~=*=~==~=*=~==~=*=~===~=*=~==~=*=~===~=*=~==~=*=~===~=*=~==~=*=~= */

#ifdef __cplusplus
}
#endif

#endif  // _ERASURECODE_H_
