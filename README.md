# dreamcast-samples
Kallistios-based dreamcast code samples.  These were intended to be accompanied by blog posts at https://numechanix.com which is currently down.

# VQFB

A VQ-texture based Framebuffer Example.  What's happening here is we allocate a VQ texure in the PVR and use it like a framebuffer with an efficient indexed color mode. This is a particularly useful optimization for procedural textures in lower color spaces.

Here we use the codebook of the VQ texture to have each entry contain runs of 4 pixels of the same color.  When we encode the image it is 4x wider than it should appear, then we sqeeze the texture horizontally in the PVR by a factor of 4 times to make it appear square.

This technique would be ideal for demoscene effects or emulators, where you generate an image in realtime in an indexed colorspace.  Provided is a very simple perlin noise visualization that uses the VQ framebuffer mode.

## Building Examples

Required:
- A working Kallistios toolchain for your system of choice
- Debugging requires GDB and dc-tool-ip installation
- CDI mastering requires installed cdrtools, cdi4dc and makeip binaries