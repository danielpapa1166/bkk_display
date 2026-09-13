# chttp

`chttp` is a minimal route-based HTTP/1.1 server library written in C. It
provides socket handling, request parsing, routing, and response generation for
the device setup web services without requiring a larger web framework.

## Integration

`chttp_git.bb` builds a pinned local checkout as a static library and installs
`libchttp.a` plus `chttp.h` in the development package. The runtime package is
intentionally empty because consumers link the library statically. The
application manager and `config-server` declare `chttp` as a build dependency
and use it to expose their local HTTP interfaces.