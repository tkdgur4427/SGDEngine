#pragma once

// define static assertion (for compile-time assertion)
#ifndef SGD_CT_ASSERT
#if !FINAL_RELEASE
#define SGD_CT_ASSERT(expression) static_assert(expression, #expression)
#else
#define SGD_CT_ASSERT(expression) __noop
#endif
#endif

// define static assertion accepting string message
#ifndef SGD_CT_ASSERT_MSG
#if !FINAL_RELEASE
#define SGD_CT_ASSERT_MSG(expression, message) static_assert(expression, message)
#else
#define SGD_CT_ASSERT_MSG(expression, message) __noop
#endif
#endif

// typical assertion leaving the message (thread-safe; asynchronous logging supported)
