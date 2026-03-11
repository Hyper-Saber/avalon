#pragma once

#ifdef AVALON_DEBUG
#define AVALON_ASSERT(expr)                                                    \
  if (!(expr)) [[unlikely]] {                                                  \
    avalon::debug::OnAssertFailed(#expr, "No extra message");                  \
  }

#define AVALON_ASSERT_MSG(expr, msg)                                           \
  if (!(expr)) [[unlikely]] {                                                  \
    avalon::debug::OnAssertFailed(#expr, msg);                                 \
  }

#define AVALON_TRACE_BACK() avalon::debug::TraceBack()
#else
#define AVALON_ASSERT(expr) ((void)0)
#define AVALON_ASSERT_MSG(expr, msg) ((void)0)
#define AVALON_TRAVALON_TRACE_BACK() ((void)0)
#endif
