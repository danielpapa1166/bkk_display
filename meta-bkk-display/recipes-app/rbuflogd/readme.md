# rbuflogd

`rbuflogd` is the project's central logging daemon and producer library.
Applications use `librbuflogd_producer.so` to emit structured log records,
while the daemon receives and persists or forwards those records according to
its upstream implementation.

## Integration

`rbuflogd_git.bb` fetches the pinned local `rbuflogd` source tree, builds it
with CMake, and compensates for the upstream CMake install gap by installing
`rbuflogd` explicitly. The package includes `/usr/bin/rbuflogd`, the producer
shared library, public headers, and `rbuflogd.service`. The unit is installed
but not automatically enabled; application-manager or deployment policy starts
it before components that produce logs. Most BKK application recipes list it
as a build and runtime dependency.