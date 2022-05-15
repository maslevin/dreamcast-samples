# Drawing Procedural Textures on the Sega Dreamcast

A problem that emulator authors have had when developing for the Sega Dreamcast, is that the memory access to VRAM is really slow compared to PC architectures from the same period in time. In FrNES, the NES emulator I wrote for the Dreamcast, this was a problem which I saw no real solution with the knowledge of the platform that we had at the time. Game console emulators generally generate a full framebuffer image for every frame of the emulated console. For the NES with its 256×256 pixel screen at 60FPS this results in about 3.8MB/s of memory bandwidth required for an 8BPP image (7.6MB/s for a 16BPP image) just to push the rendered image to VRAM for display. The NES and most 8-bit game consoles only need to display 256 or fewer onscreen colours so using a 16-bit per pixel onscreen representation is wasteful and unnecessary.

One core issue is that the PVR chip in the Dreamcast doesn’t have an 8BPP display mode, so you can’t directly draw 8BPP data to the screen. The PVR has an 8BPP paletted texture format, which at first glance seems an obvious choice for this application. However a side effect of 8BPP (and all paletted textures on the PVR) is that they must be stored in twiddled format when they are loaded into the PVR. Twiddled memory format stores contiguous pixels in a non-linear arrangement (basically a series of nested horizontally flipped “N”s in memory) which allows the texture to be bilinearly filtered by the hardware for free. Many games use this format, storing their textures in the .pvr image format which pre-twiddles the texture in storage. This is great for applications which don’t procedurally generate images in code.

Emulators rebuild their framebuffer texture every frame and twiddling the image at runtime before sending it to the PVR is not an option (the memory bandwidth gains of going to 8BPP aren’t worth the extra CPU overhead of twiddling the image in memory beforehand) so paletted textures are a nonstarter for this particular application. This was my conclusion in 2001 when I stopped working on FrNES.

## VQ To the Rescue

VQ Textures are the compressed texture format of the PVR chip. The format of the texture starts with a 2048-byte codebook, which is interpreted as 256 runs of 4 contiguous 16-bit pixels (any of the 16-bit pixel formats will work). The texture data is then represented as an 8 bit index into the codebook. The twiddled VQ format differs by representing each codebook entry as a horizontally flipped “N” of pixels, instead of a lengthwise run.

We can use the VQ texture format to do exactly what we need. By using the non-twiddled version of a VQ texture, and setting all 4 entries in each codebook entry to being the same pixel, each pixel in our 256×256 texture can be represented as a byte-length index into our codebook. This effectively gives us a 256×256 8BPP texture with it’s own 2kB palette, with the caveat that the texture itself is seen as 4x as wide as we had previously specified (as far as the PVR is concerned it’s a 1024×256 texture, because each codebook entry is four pixels wide). One downside to this approach is that it doesn’t scale any higher than 256 pixel wide textures (as the maximum sized PVR texture is 1024 pixels wide) however it’s totally possible spread a framebuffer across multiple textures to get the desired effect.

Other approaches exist as well, using the codebook entries to encode patterns instead of single colours, and managing the entries carefully to encode the desired pattern, opening the door to 2 or 4 bpp framebuffers as well.

## Memory Transfer Options

The following memory transfer options are available.

- Direct Access:
You can directly address VRAM with the CPU and write to the framebuffer (in non-PVR mode) or a texture (in PVR-mode). This is the slowest method of writing to VRAM.

- Store Queues:
Store Queues are a feature of the SH-4 CPU which allows 64-byte chunks to be moved from the queues (memory / registers on the CPU itself) to a memory location (even VRAM) at essentially no cost. This CPU feature is easy to use and was used to great effect in the later builds of FrNES to move the framebuffer texture to VRAM so it could be rendered. The KOS function pvr_txr_load uses Storage Queues to move data from main memory to VRAM. Greater efficiency can be gained by directly rendering to the Store Queues and sending those 64-byte chunks to VRAM as soon as they are available instead of rendering to an intermediate buffer which will later be copied.

- DMA Transfer:
DMA transfers are the fastest way of moving data from main memory to VRAM. This strategy is best used when your whole texture has been loaded up front (right after it’s been decompressed from a file etc.) and is ready to move to VRAM all at once. Some threading and synchronization constraints must be observed to use PVR DMA successfully. You could use the KOS function pvr_dma_transfer for this purpose.

[Note: dcemulation.org forum poster TapamN posted an example of using a VQ texture for this purpose way back in 2013 which lead me to investigate the approach and write my own example. Major props to TapamN for publishing his code and idea!]
[Note: Most emulators don’t correctly implement non-twiddled VQ textures. Lxdream incorrectly calculates texture width of non-twiddled VQ textures by a factor of 2]

## Perlin Noise Example

https://youtu.be/m4IsaJGTwg8

Because I don’t have an emulator to hook this up to presently, I thought I would present a self-contained example that shows the paletted VQ texture. I hooked up Sebastien Rombauts great simplex noise implementation as a texture generator to the 8bpp texture approach I took. Check it out! I have artificially changed the horizontal scaling factor to make it render correctly in lxdream. Lxdream is also a little bit stuttery but on a real Dreamcast, this animation runs at 60FPS.