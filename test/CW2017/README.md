
# CW2017

A Tango demonstration case for CW2017. This consists of three models at different resolutions passing 10 fields in a single direction. Atmosphere is approx 2 degrees, ice is 1/4 degree and ocean is 1/10 the degree. The atmosphere is written in Python, the others are in C++.

The demo shows the simplicity of Tango configuration as well as start-up and runtime performance. A similar configuration has been developed using OASIS3-MCT for comparison purposes.

## To build

## To run

## Features

- Very easy to understand and use. This is the main point.
- Supports *any* domain decomposition, detects invalid/overapping domains.

## Design

- Arbitrary mappings.

## Performance Observations

With the configurations I've worked on computational performance of the coupler is not a big issue. As long as fully distributed coupling is supported.

Startup performance is / is not an issues for larger grids?

For higher core counts where tiles are smaller the cost of applying the weights matrix decreases. The cost of MPI becomes the dominant factor. Tango is being optimized for this use case.

As a generalization there are more coupling fields going 'down' from atmosphere to ocean than the other way. This means that we are more often sending from low resolutions to higher ones. This has the following implications:

1. If comms bandwidth is a limitation then it makes most sense to do the remapping on the fine grid (because smaller messages will be sent).
2. If comms latency is the bottle-neck then do the remapping on the least busy side (and use asynchronous send so all sends are queued).
3. If cpu is the bottle-neck (unlikely!) do as above.

Since the finer grids generally have higher core counts it makes sense from an efficiency point of view to make sure these models do not wait. If the coarser grids are doing the waiting then they should do the remapping, queue up a send and be waiting on a receive.

Presently Tango defaults to applying the remapping on the send side.

## Observations


