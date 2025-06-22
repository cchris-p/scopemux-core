/**
 * @file crash_handler.h
 * @brief Signal handling and crash protection utilities
 *
 * This module provides signal handlers to catch and diagnose crashes
 * like segmentation faults, particularly those that might occur during
 * Python garbage collection due to circular references.
 */

#ifndef SCOPEMUX_CRASH_HANDLER_H
#define SCOPEMUX_CRASH_HANDLER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Crash handler configuration options
 */
typedef struct {
    bool log_backtrace;       /**< Generate backtrace information when crashes occur */
    bool handle_segv;         /**< Install handler for SIGSEGV */
    bool handle_abrt;         /**< Install handler for SIGABRT */
    bool handle_fpe;          /**< Install handler for SIGFPE */
    bool handle_bus;          /**< Install handler for SIGBUS */
    bool handle_ill;          /**< Install handler for SIGILL */
    bool attempt_recovery;    /**< Attempt to recover from non-fatal crashes */
    bool fail_safety;         /**< When true, fail operations safely on error instead of crashing */
    const char* crash_log_path; /**< Path to write crash logs (NULL for stderr) */
} CrashHandlerConfig;

/**
 * @brief Initialize crash handling system
 * 
 * Install signal handlers to catch and diagnose crashes.
 * 
 * @param config Configuration options for the crash handler
 * @return true if handlers were successfully installed, false otherwise
 */
bool crash_handler_init(const CrashHandlerConfig* config);

/**
 * @brief Clean up crash handling system
 * 
 * Restore original signal handlers.
 */
void crash_handler_cleanup(void);

/**
 * @brief Generate a backtrace at the current point
 * 
 * Useful for debugging to see how execution reached a certain point.
 * 
 * @param max_frames Maximum number of frames to capture in the backtrace
 */
void crash_handler_print_backtrace(int max_frames);

/**
 * @brief Check if pointer is likely to cause a crash
 * 
 * Performs various safety checks on a pointer to determine if
 * dereferencing it would likely cause a crash.
 * 
 * @param ptr Pointer to check
 * @return true if pointer appears valid, false if it could cause a crash
 */
bool crash_handler_is_safe_ptr(const void* ptr);

/**
 * @brief Safely dereference a pointer with crash protection
 * 
 * Checks if dereferencing a pointer would cause a crash and returns
 * a fallback value if unsafe.
 * 
 * @param ptr Pointer to safely dereference
 * @param fallback Value to return if ptr is unsafe
 * @return Value at ptr if safe, fallback otherwise
 */
void* crash_handler_safe_deref(void* ptr, void* fallback);

/**
 * @brief Set thread name for better crash diagnostics
 * 
 * Sets the thread name which will appear in crash reports.
 * 
 * @param name Name to assign to the current thread
 */
void crash_handler_set_thread_name(const char* name);

/**
 * @brief Push context information for crash reports
 * 
 * Adds context information that will be included in crash reports.
 * Useful for including what the code was doing when it crashed.
 * 
 * @param context_info Description of current operation context
 * @return Context ID that can be used to pop this context later
 */
uint32_t crash_handler_push_context(const char* context_info);

/**
 * @brief Pop context information
 * 
 * Removes context information previously pushed.
 * 
 * @param context_id ID returned by crash_handler_push_context
 */
void crash_handler_pop_context(uint32_t context_id);

/**
 * @brief Register a function to be called when a crash occurs
 * 
 * @param callback Function to call when crash occurs
 * @param user_data User data to pass to callback
 * @return Registration ID that can be used to unregister
 */
uint32_t crash_handler_register_callback(void (*callback)(void*), void* user_data);

/**
 * @brief Unregister a previously registered crash callback
 * 
 * @param registration_id ID returned by crash_handler_register_callback
 */
void crash_handler_unregister_callback(uint32_t registration_id);

/**
 * @brief Get default crash handler configuration with safe defaults
 * 
 * @return CrashHandlerConfig with safe default values
 */
CrashHandlerConfig crash_handler_get_default_config(void);

#endif /* SCOPEMUX_CRASH_HANDLER_H */
