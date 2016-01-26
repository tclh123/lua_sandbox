/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** Heka sandbox implementation @file */

#ifndef luasandbox_heka_sandbox_sandbox_h_
#define luasandbox_heka_sandbox_sandbox_h_

#include <stdbool.h>
#include <time.h>

#include "luasandbox.h"
#include "luasandbox/lua.h"
#include "luasandbox/lauxlib.h"
#include "luasandbox/util/heka_message.h"

#define LSB_HEKA_MAX_MESSAGE_SIZE "max_message_size"

enum lsb_heka_pm_rv {
  LSB_HEKA_PM_SENT  = 0,
  LSB_HEKA_PM_FAIL  = -1,
  LSB_HEKA_PM_SKIP  = -2,
  LSB_HEKA_PM_RETRY = -3,
  LSB_HEKA_PM_BATCH = -4,
  LSB_HEKA_PM_ASYNC = -5
};

typedef struct lsb_heka_sandbox lsb_heka_sandbox;

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * inject_message callback function provided by the host. Only one (or neither)
 * of the checkpoint values will be set in a call.  Numeric checkpoints can have
 * a measurable performance benefit but are not always applicable so both option
 * are provided to support various types of input plugins.
 *
 * @param parent Opaque pointer the host object owning this sandbox
 * @param pb Pointer to a Heka protobuf encoded message being injected.
 * @param pb_en Length of s
 * @param cp_numeric Numeric based checkpoint (can be NAN)
 * @param cp_string String based checkpoint (can be NULL)
 *
 * @return 0 on success
 */
typedef int (*lsb_heka_inject_message_input)(void *parent,
                                             const char *pb,
                                             size_t pb_len,
                                             double cp_numeric,
                                             const char *cp_string);

/**
 * inject_message callback function provided by the host.
 *
 * @param parent Opaque pointer the host object owning this sandbox.
 * @param pb Pointer to a Heka protobuf encoded message being injected.
 * @param len Length of pb_len
 *
 * @return 0 on success
 */
typedef int (*lsb_heka_inject_message_analysis)(void *parent,
                                                const char *pb,
                                                size_t pb_len);

/**
 * update_checkpoint callback function provided by the host.
 *
 * @param parent Opaque pointer the host object owning this sandbox.
 * @param sequence_id Opaque pointer to the host message sequencing (passed into
 *               process_message).
 *
 * @return 0 on success
 */
typedef int (*lsb_heka_update_checkpoint)(void *parent, void *sequence_id);

/**
 * Create a sandbox supporting the Heka Input Plugin API
 *
 * @param parent Opaque pointer the host object owning this sandbox
 * @param lua_file Fully qualified path to the Lua source file
 * @param state_file Fully qualified filename to the state preservation file
 *                   (NULL if no preservation is required)
 * @param lsb_cfg Full configuration string as a Lua table (NULL for lsb
 *                defaults)
 * @param logger Logger call back (NULL to disable logging)
 * @param im inject_message call back
 * @return lsb_heka_sandbox* On success a pointer to the sandbox otherwise NULL
 */
LSB_EXPORT
lsb_heka_sandbox* lsb_heka_create_input(void *parent,
                                        const char *lua_file,
                                        const char *state_file,
                                        const char *lsb_cfg,
                                        lsb_logger logger,
                                        lsb_heka_inject_message_input im);

/**
 * Host access to the input sandbox process_message API.  If a numeric
 * checkpoint is set the string checkpoint is ignored.
 *
 * @param hsb Heka input sandbox
 * @param msg Heka message to process
 * @param cp_numeric NAN if no numeric checkpoint
 * @param cp_string NULL if no string checkpoint
 * @param profile Take a timing sample on this execution
 *
 * @return int
 *  >0 fatal error
 *  0  success
 *  <0 non-fatal error (status message is logged)
 *
 */
LSB_EXPORT
int lsb_heka_process_message_input(lsb_heka_sandbox *hsb,
                                   lsb_heka_message *msg,
                                   double cp_numeric,
                                   const char *cp_string,
                                   bool profile);

/**
 * Create a sandbox supporting the Heka Analysis Plugin API
 *
 * @param parent Opaque pointer the host object owning this sandbox
 * @param lua_file Fully qualified filename to the Lua source file
 * @param state_file Fully qualified filename to the state preservation file
 *                   (NULL if no preservation is required)
 * @param lsb_cfg Full configuration string as a Lua table (NULL for lsb
 *                defaults)
 * @param logger Logger call back (NULL to disable logging)
 * @param im inject_message call back
 * @return lsb_heka_sandbox* On success a pointer to the sandbox otherwise NULL
 */
LSB_EXPORT
lsb_heka_sandbox* lsb_heka_create_analysis(void *parent,
                                           const char *lua_file,
                                           const char *state_file,
                                           const char *lsb_cfg,
                                           lsb_logger logger,
                                           lsb_heka_inject_message_analysis im);

/**
 * Host access to the analysis sandbox process_message API
 *
 * @param hsb Heka analysis sandbox
 * @param msg Heka message to process
 * @param profile Take a timing sample on this execution
 *
 * @return int
 *  >0 fatal error
 *  0  success
 *  <0 non-fatal error (status message is logged)
 *
 */
LSB_EXPORT
int lsb_heka_process_message_analysis(lsb_heka_sandbox *hsb,
                                      lsb_heka_message *msg,
                                      bool profile);

/**
 * Create a sandbox supporting the Heka Output Plugin API
 *
 * @param parent Opaque pointer the host object owning this sandbox
 * @param lua_file Fully qualified path to the Lua source file
 * @param state_file Fully qualified filename to the state preservation file
 *                   (NULL if no preservation is required)
 * @param lsb_cfg Full configuration string as a Lua table (NULL for lsb
 *                defaults)
 * @param logger Logger call back (NULL to disable logging)
 * @param upc checkpoint_updated call back when using batch or async output
 *
 * @return lsb_heka_sandbox* On success a pointer to the sandbox otherwise NULL
 */
LSB_EXPORT
lsb_heka_sandbox* lsb_heka_create_output(void *parent,
                                         const char *lua_file,
                                         const char *state_file,
                                         const char *lsb_cfg,
                                         lsb_logger logger,
                                         lsb_heka_update_checkpoint ucp);

/**
 * Host access to the output sandobx process_message API
 *
 * @param hsb Heka output sandbox
 * @param msg Heka message to process
 * @param sequence_id Opaque pointer to the message sequence id (only used for
 *                    async output plugin otherwise it should be NULL)
 * @param profile Take a timing sample on this execution
 *
 * @return int
 *  >0 fatal error
 *  0  success
 *  -1 non-fatal_error (status message is logged)
 *  -2 skip
 *  -3 retry
 *  -4 batching
 *  -5 async output
 *
 */
LSB_EXPORT
int lsb_heka_process_message_output(lsb_heka_sandbox *hsb,
                                    lsb_heka_message *msg,
                                    void *sequence_id,
                                    bool profile);

/**
 * Aborts the running sandbox from a different thread of execution. A "shutting
 * down" termination message is generated. Used to abort long runnning sandboxes
 * such as an input sandbox.
 *
 * @param hsb Heka sandbox to abort
 *
 * @return
 *
 */
LSB_EXPORT void
lsb_heka_stop_sandbox(lsb_heka_sandbox *hsb);

/**
 * Terminates the sandbox as if it had a fatal error (not thread safe).
 *
 * @param hsb Heka sandbox to terminate
 * @param err Reason for termination
 */
LSB_EXPORT void
lsb_heka_terminate_sandbox(lsb_heka_sandbox *lsb, const char *err);

/**
 * Frees all memory associated with the sandbox; hsb cannont be used after this
 * point and the host should set it to NULL.
 *
 * @param hsb Heka sandbox to free
 *
 * @return NULL on success, pointer to an error message on failure that MUST BE
 * FREED by the caller.
 *
 */
LSB_EXPORT char *
lsb_heka_destroy_sandbox(lsb_heka_sandbox *hsb);

/**
 * Host access to the timer_event API
 *
 * @param hsb Heka sandbox
 * @param t Clock time of the timer_event execution
 * @param shutdown Flag indicating the Host is shutting down allowing the
 *                 sandbox to do any desired finialization)
 *
 * @return int 0 on success
 */
LSB_EXPORT
int lsb_heka_timer_event(lsb_heka_sandbox *hsb, time_t t, bool shutdown);


/**
 * Return the last error in human readable form.
 *
 * @param hsb Heka sandbox
 *
 * @return const char* error message
 */
LSB_EXPORT const char* lsb_heka_get_error(lsb_heka_sandbox *hsb);

/**
 * Returns the filename of the Lua source.
 *
 * @param hsb Heka sandbox
 *
 * @return const char* filename.
 */
LSB_EXPORT const char* lsb_heka_get_lua_file(lsb_heka_sandbox *hsb);

// todo need access to the sandbox statistics.  For simplicity it will most
// likely return a Heka protobuf string with all the info

#ifdef __cplusplus
}
#endif

#endif