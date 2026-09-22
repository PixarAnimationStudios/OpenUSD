# hgiWebGPU

hgiWebGPU is a WebGPU implementation of Hydra Graphics Interface (HGI). It
can be used as a native backend using Google's Dawn library as the implementation
of WebGPU or compiled through Emscripten, in which case, the WebGPU implementation
will be defined by the browser.

# Limitations

- `MultidrawIndirect` now available in WebGPU dawn `chromium/6990` and available as experimental
feature since Chrome 131 but not implemented yet.
- GPU Frustum cull not supported due to the use of `isinf` in the shader and potential
issues with `indirectDraw` support.
- Packed normals disabled due to missing support for format `HgiFormatPackedInt1010102`.
Currently, WebGPU only supports `Unorm10_10_10_2`. 
- 4GB of memory limitation in WebAssembly (potentially fixed with [memory64](https://github.com/WebAssembly/memory64)).
This feature has already been introduced to Chrome, but it has an impact on performance,
so 32-bit is still the default.
- Only support the `DRAW_SHADED_SMOOTH` draw mode.
-  Not possible to pass write buffers to the vertex stage (e.g. `constPrimVars`) (as defined in
[`createBindGroupLayout`](https://www.w3.org/TR/webgpu/#dom-gpudevice-createbindgrouplayout) in the device timeline section).
- Only vertex, fragment and compute shaders available.
- Missing builtins available in GLSL but not in WGSL:
  - `determinant`, it could potentially be polyfilled.
  - `isinf`
  - `isnan`
  - `barycentric coords`
  - `gl_Pointsize` is always 1 and cannot be modified.
  - `gl_BaseInstance`, although it can be worked around as far as we know.
- No support for synchronous read-back of textures. For example, in the picking task.
Some workarounds involve a busy wait, but for Emscripten this requires including
`ASYNCIFY`/`asyncify.js`, which is unsuitable for production applications.

- There are a couple of limits specified by the WebGPU API that are reached under certain configurations.
  For example, when using the OIT task, the data `BufferArrayRange` can exceed the maximum buffer size that can
  be requested for WebGPU on some high-definition displays. Similarly, when having a model that has
  instancing over a translucent material, the number of bound buffers will reach
  the WebGPU limit, with all 10 out of 10 storage buffers being used.

- After conversion to WebAssembly, the WASM package has a 90MB size. There
are two problems that  make it challenging to reduce the size:
  -  In WASM, we have to create a monolithic package, which includes all the libraries and
possible plugins that the final application will use. This could be improved by using dynamic libraries
and loading them at runtime as needed from the server.
  -  We cannot apply `-O3` optimizations to the code, because it removes some of the symbols that
should be kept. This can be solved by tagging the functions that should not be deleted with the `EMSCRIPTEN_KEEPALIVE`
- Early fragment test will not be executed if the shader writes to storage, based
on the [Vulkan specification](https://docs.vulkan.org/features/latest/features/proposals/VK_AMD_shader_early_and_late_fragment_tests.html#_problem_statement)
but [WebGPU has no way to force it](https://groups.google.com/g/webgl-dev-list/c/nG7yEjCHxGI/m/l_ZmIr9DAgAJ).
To work around this, we introduced the `HgiDeviceCapabilitiesForceEarlyFragmentTest` capability to
explicitly test in the fragment shader.
- No ray tracing support for [Autodesk/Aurora](https://github.com/Autodesk/Aurora).
- No support for push constants. Only available with a "polyfill" that could potentially degrade performance, since
it recreates buffers every time a push constant is updated.
- WebGPU only has optional support for `ClipDistance`s. We introduced the
`HgiDeviceCapabilitiesClipDistance` capability to make it optional.
Clip distances are simply ignored when unsupported.
- Some rendering representations are not completely supported

| Representation | Note |
|----------------|------|
| Smooth | Supported |
| Points | Only basic point rendering supported, no custom point sizes |
| Wireframe | Supported through line fallback, no custom line width |
