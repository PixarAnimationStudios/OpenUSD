# Sdr: Shader Definition Registry {#sdr_page_front}
\if ( PIXAR_MFB_BUILD )
\mainpage Sdr: Shader Definition Registry
\endif

Sdr provides an extendable framework to discover, lazily parse,
and ask for information about shaders.

### Background

A shader is a program that takes input and produces output. These programs
can be implemented in a variety of different languages with different runtimes.

Rather than interface with each shading system's representation's
idiosyncracies directly, complex pipelines prefer to hide away this information
(but otherwise keep the information accessible) until the pipeline decides to
run the shader.

With Sdr, pipelines need only hold on to two pieces of data
-- `identifier` and `shadingSystem` -- to find the following:
- the original shader implementation
- input and output properties of the shader
- Sdr-standardized common metadata and custom metadata, for both items above

### Usage

<ol>
<li> \subpage Sdr_Registry </li>
<ol type="i">
    <li> \ref Sdr_Registry_Discovery</li>
    <li> \ref Sdr_Registry_Parse</li>
    <li> \ref Sdr_Registry_Queries</li>
</ol>
<li> \subpage Sdr_ShaderNode </li>
<li> \subpage Sdr_ShaderProperty </li>
</ol>