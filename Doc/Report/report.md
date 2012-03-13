% Deferred Shading 
% Albert Cervin

# Abstract
In this paper, a rendering algorithm called deferred shading is
presented. Deferred shading is an algorithm used extensively in modern
game engines and real-time applications. The algorithm decouples
object and material properties from lighting calculations, removing
many unnecessary lighting calculations. This is done by performing
geometric calculations and lighting calculations in separate passes.
This means that deferred rendering is a multi-pass algorithm. In the
first step, geometric information is stored in what is called a
G-Buffer. In the second step, the geometric information in the
G-Buffer is used to perform lighting calculations and produce a final
pixel color. The result is an implementation
of a deferred shading algorithm in DirectX 10. The implementation
performs well for a large number of dynamic light sources as can be
expected for a deferred rendering algorithm.

# Introduction 
Deferred shading is an old idea that was proposed by
Deering et al. [@Deering:1988] in 1988. However, it has not been used extensively until recent
years. It was proposed for usage in games by Hargreaves [@Hargreaves]
in 2004 and today, deferred shading is a de facto standard in game engines
and also in other types of real-time rendering applications. The need for deferred
shading arises in scenes with many dynamic light sources. In these
cases deferred shading can simplify and speed up lighting calculations
by orders of magnitude. There are downsides, however and the memory
bandwidth requirement of the algorithm is high, even with modern
graphics cards.

Deferred shading is also known under names such as deferred rendering
and deferred lighting. However, all three names essentially describes
the same algorithm.

## Alternatives to deferred shading
The "opposite" of deferred shading is forward rendering which can be
performed in different ways.

In games and other real-time applications, many different types of
light sources can be applied to objects with a variety of materials.
This gives a lot of combinations (Half Life 2 has 1920 pixel shader
combinations [@McTaggart]). If deferred shading is not used, the
most straightforward approach is to loop over all lights in the
material shaders. However, shader branching performs poorly on current
hardware. The solution to this is to compile a different shader for
each type of combination. This can be achieved using language features
or code preprocessors. The approach is called über-shaders [@Melax]
[@Trapp] and this is the way games have tackled the problem historically.

Another alternative is to use a multi-pass lighting
approach. The idea is to perform one rendering pass per light and use
the hardware blending capabilities to accumulate light contributions.
To do this, the lights affecting a particular object is determined. To
narrow down the number of lights affecting an object, attenuation is
used for the light sources. The obvious problem with this approach is
that the process is $O(m*n)$. This means that making the algorithm
perform good in large scenes is not at all trivial. Efficiency can for
example be improved by scissoring in cases where lights affects only parts of
an object.

Other practical issues with the algorithm include the high memory
bandwidth requirements for using hardware blending and that objects
need to be run through the vertex shader multiple times.

In comparison with the über-shader approach, the multi-pass approach
gives a lower number of total shaders. To add a new light type, one
shader for each type of material, affected by this new light source has to be implemented.

# Method

## The G-buffer 
Before any rendering happens, the scene contains
geometric information. In the classic rendering approach, called
forward rendering, each object is rendered and for each object,
lighting calculations are performed. The problem with this arises when
there are many dynamic light sources. As mentioned above, the most common technique for
handling this, historically, is to use what is called über-shaders.
However, as stated, über-shaders does not scale well with the number
of light/material combinations and introduces a far from ideal
situation with shader permutations.

Deferred shading tackles this problem by _deferring_ the lighting
calculations to a later stage in the algorithm. This makes deferred
shading a multi-pass algorithm and means that the hardware has to
support multiple render targets (often referred to as MRT). The
multiple render targets are used for, in the first pass of the
algorithm, storing geometric scene information. The geometric
information is typically at least view space normals and depth.

The buffer (essentially a collection of render targets) is called a
G-Buffer (short for geometry buffer) and this stage of the algorithm
is called geometry stage. Vertex normals are transformed into
view-space by multiplying with a normal matrix that is the inverse
transpose of the upper $3x3$ section of the matrix $world*view$.

\begin{equation} 
	\text{vs\_normal} = \text{world\_view\_inv} *
	\text{vertex\_normal} 
	\label{eq:vs_normals} 
\end{equation}

To have sufficient precision in the normals for later calculations it
is possible to use 16-bit textures to store the normals. This is
however both wasteful and inefficient on modern hardware. The normals
can therefore be transformed into sphere map coordinates and then
stored in an 8-bit texture. 

## Compact normal storage for G-Buffers
Spherical environment mapping is a mapping that indirectly maps a
reflection vector to texture coordinates. A good property of this
transform is that the reflection vector can point away from the
camera which is also true for the view space normal [@Aras_Normals].

To do this transformation, Lambert azimuthal equal-area projection can
be used. This transformation and the inverse of it are described as

\begin{align} 
    &(X, Y) = \left( \sqrt{\frac{2}{1-z}x},\sqrt{\frac{2}{1-z}y} \right) \nonumber \\
    &(x, y, z) = \left( \sqrt{1 - \frac{X^2 + Y^2}{4}X},
    \sqrt{\frac{X^2 + Y^2}{4}Y}, \right. \nonumber \\ 
    & \left. -1+\frac{X^2 + Y^2}{2} \right)
    \label{eq:lambert} 
\end{align}

where $(X, Y)$ is the resulting encoded normal. Assuming the normal is
normalized, this can be implemented in HLSL like

    half2 encode (half3 n, float3 view)
    {
        half f = sqrt(8*n.z+8);
        return n.xy / f + 0.5;
    }
    
    half3 decode (half4 enc, float3 view)
    {
        half2 fenc = enc*4-2;
        half f = dot(fenc,fenc);
        half g = sqrt(1-f/4);
        half3 n;
        n.xy = fenc*g;
        n.z = 1-f/2;
        return n;
    }.
    
This compression gives a very small error [@Aras_Normals] compared to the "exact" 16-bit normal storage
and can be used to save bandwidth and GPU resources. However, it must
be noted that an 8-bit representation of the view space normal might
be too conservative in some cases and 16 bit storage has to be used.

Depth can also be stored in the G-Buffer. It is however possible to
use the already existing depth buffer generation and bind the depth
buffer as a shader resource in later stages. One important property of
the depth buffer generated by hardware is that it contains projected
depth values, that is, clip space depth. To use this depth in
calculations it has to be unprojected by dividing it with the distance
to the far clip plane.

The depth can furthermore be used to reconstruct view space positions
which means that the positions does not need to be stored in the
G-Buffer.

After this step, the geometry in the scene itself is not needed
anymore and no actual objects are pushed through the graphics
pipeline.

A weakness with the algorithm shows up in the G-Buffer step, however.
The fill rate costs for writing the G-Buffers are significant and
this is especially true for console hardware.

Another fairly big disadvantage is that deferred shading does not
support hardware antialiasing. Antialiasing has to be done with
methods as MSAA which can be very slow.

## The lighting stage 
When all needed geometric information has been
stored in the G-Buffer it can be used in the lighting stage. This is
done by binding the G-Buffer render targets as shader resources.

The geometric information is then used to calculate lighting in any
way that fits the application. Phong shading can for example be
implemented. Since the resources in the G-Buffer are two-dimensional
textures, lighting calculations are done by drawing a full screen
triangle and point sampling the G-Buffer textures.

At this stage it is easy to see that the number of lighting
calculations decreases. Consider a scene where there is $N$ light
sources and $M$ objects. With classic forward rendering, the
contribution from each light source has to be calculated for each object,
resulting in $O(N*M)$ calculations. With the deferred approach, the $M$
objects are first rendered into the G-Buffer and in the lighting
stage, one fullscreen triangle for each of the $N$ lights is
rendered. This means that the light calculation complexity of the
deferred shading algorithm is $O(N+M)$.

Another performance gain in deferred shading comes from the fact that
GPUs render faster when the same shader is used for many objects. The
fact that the same shader can be used for the whole G-Buffer
generation and that lighting shaders can be used for a longer string
of objects, will speed up the rendering.

## Implementation 
The deferred shading algoritm has been implemented
in the DirectX SDK. The minimum required DirectX version is 10 which
means that a computer running at least Windows Vista is required to
run the sample.

The implementation uses 16 bit floating point textures (render
targets) to achieve the maximum visual quality. The X, Y and Z
values of the view space normals are stored in the R, G and B channels
of one render target, respectively.

Furthermore, the implementation stores depth in one 16 bit G-buffer
channel. This is a bit low resolution for depth (it is usually stored
in 24 bits) but it is only used to
reconstruct view space position which is, as said,
reconstructed instead of being passed on in the G-buffer (described in
detail in section \ref{sec:depth}).

### G-Buffer layout
The G-Buffer layout for the implementation is presented below.

\begin{tabular}{|m{1.5cm}|m{1.5cm}|m{1.5cm}|m{1.5cm}|}
  \hline
  R & G & B & A \\
  \hline
  Normal.x & Normal.y & Normal.z & Not used \\
  \hline
  Depth & Specular intensity & Specular roughness & Not used \\
  \hline
  Albedo.x & Albedo.y & Albedo.z & Not used \\
  \hline
\end{tabular}

This layout could be designed more efficiently but the empty slots are
kept for future needs. Albedo in this case is the diffuse color
provided by textures or color parameters for the object. An example of
G-Buffer render targets is presented in figures \ref{fig:normals} to \ref{fig:spec_i}.

\begin{figure*}
    \centering
    \includegraphics[width=8cm]{figures/screenshot_3}
    \caption{View space normals stored in RT0 (Render Target 0).}
    \label{fig:normals}
\end{figure*}

\begin{figure*}
    \centering
    \includegraphics[width=8cm]{figures/screenshot_5}
    \caption{Diffuse Albedo color stored in RT2.}
    \label{fig:albedo}
\end{figure*}

\begin{figure*}
    \centering
    \includegraphics[width=8cm]{figures/screenshot_4}
    \caption{View space depth stored in RT1. (Brightness and contrast
    have been altered for better visibility.)}
    \label{fig:depth}
\end{figure*}

\begin{figure*}
    \centering
    \includegraphics[width=8cm]{figures/screenshot_7}
    \caption{Specular intensity stored in RT1.}
    \label{fig:spec_i}
\end{figure*}

### Reconstructing view space position from depth
\label{sec:depth}
Since the implementation stores view space depth, there is no need to
project and unproject the value as would be the case if the hardware
depth buffer had been used. Instead the approach suggested by Wenzel
[@Wenzel] is used. A ray is constructed pointing from the camera to the
far-clip plane and then multiplied by the depth value. In HLSL code
this looks like

	float depth = 
		Depth.Load(
            float3(Input.Position.xy, 0.f)).r;
            
	float4 position = 
		float4(depth * Input.FrustumCorner, 1.f);
	
where `Input.FrustumCorner` is the view space frustum corner
corresponding to this vertex. Since a full screen quad is used, each
vertex is assigned a frustum corner position as a texture coordinate
which means that there is always an interpolated ray from the camera
to the far plane (shown in figure \ref{fig:view_ray}).

\begin{figure}
    \centering
    \includegraphics[width=7cm]{figures/view-ray}
    \caption{Reconstructing view space position from view space
    depth.}
    \label{fig:view_ray}
\end{figure}

The problem remaining is how to obtain the far plane corners from the
frustum. This can be done from the view matrix.

The far clipping plane is defined as

\begin{align} 
    &\text{far\_plane.normal} =\nonumber \\ &(\text{view\_matrix}(1, 3),
    \text{view\_matrix}(2, 3),\nonumber \\ &\text{view\_matrix}(3, 3))
    \label{eq:far_plane} 
\end{align}

\begin{equation} 
	\text{far\_plane.d} = \text{view\_matrix}(4, 4) - \text{view\_matrix}(4, 3)
	\label{eq:far_plane_d} 
\end{equation}

This is then normalized and intersected with the other planes as
appropriate to obtain the far plane corners. Other planes are obtained
in a similar way as described in equations \ref{eq:far_plane} and \ref{eq:far_plane_d}.

The implementation also contains code for loading Wavefront OBJ models and
for handling lighting and scene setup. Currently it is possible to use
spot and directional lights.

## Architectural benefits
From an implementation point of view, deferred shading also provides
some architectural benefits. This is since the algorithm offers a
strong separation between the definition of lighting and
materials. The materials and material properties are in large part
handled in the G-Buffer step whereas the lights are handled in the
lighting step.

# Results 
The result is an implementation of a deferred shading
algorithm in the DirectX SDK. The implementation runs in real-time and
rendering statistics for the algorithm are presented in table \ref{tab:perf}.

\begin{figure}
    \centering
    \includegraphics[width=7cm]{figures/screenshot_8}
    \caption{Final composit. (Corresponding to figure \ref{fig:spec_i})}
    \label{fig:composit1}
\end{figure}

\begin{figure}
    \centering
    \includegraphics[width=7cm]{figures/screenshot_6}
    \caption{Final composit. (Corresponding to figures
    \ref{fig:normals} - \ref{fig:depth})}
    \label{fig:composit2}
\end{figure}

\begin{figure}
    \centering
    \includegraphics[width=7cm]{figures/screenshot_10}
    \caption{Final composit.}
    \label{fig:composit3}
\end{figure}

\begin{table}
\centering
\begin{tabular}{|m{1.5cm}|m{1.5cm}|m{1.5cm}|m{1.5cm}|}
  \hline
  GPU & Test case 1 (figure \ref{fig:composit1}) & Test case 2 (figure
  \ref{fig:composit2}) & Test case 3 (figure \ref{fig:composit3}) \\
  \hline
  NVidia GeForce 9600GT & 273 FPS & 275 FPS & 273 FPS \\
  \hline
  NVidia GeForce 310M & 60 FPS & 59 FPS & 58 FPS \\
  \hline
\end{tabular}
\caption{Performance figures.}
\label{tab:perf}
\end{table}

It can be seen in table \ref{tab:perf} that a quite high polygon count runs well
even on moderate notebook GPUs. It can also be seen that an increase
in the number of lights (from two in figure \ref{fig:composit2} to ten
in figure \ref{fig:composit1}), does not affect the performance as could be
expected in a forward renderer. Since the application is almost
totally GPU bound, there is no reason to compare computers according
to CPU. All cases in table \ref{tab:perf} contains $364091$ polygons (since no
view frustum culling is performed). The small variations in FPS are likely
due to the amount of geometry undergoing visibility and backface culling.

# Discussion 
The implementation works satisfying and it is efficient
as expected. To fully leverage all advantages of the algorithm, some
implementation of light scissoring is needed. Andersson [@Andersson] proposes
a solution to this problem by dividing the screen into tiles and
calculate which lights contribute to which tiles. This can be
implemented with the help of compute shaders, a feature available in
DirectX 11. In this compute shader step, light culling is performed by
determining visible light sources for each tile. The compute shader
step then results in a number of visible light sources and an index list
of visible light sources per tile. This is then accumulated and
combined with shading albedos to produce the final pixel color.

Deferred shading is also a very good platform for various
post-processing effect since there is already a texture resource
containing the raw image before post processing. If the time would
have allowed, I would have implemented some post processing effects
like depth of field, motion blur and tonemapping. Since 16 bit
floating point textures are used, it is absolutely neccessary to use
tonemapping to get correct results with this dynamic range.

I have learned a lot in implementing this project, much due to the fact
that the DirectX API offers more manual control than the OpenGL 2 API
which has been used exclusively earlier in the education. This means
that some fundamental math for computer graphics had to be revisited
and refreshed, which is always good.

It would have been very interesting to have access to DirectX 11
hardware to be able to implement more advanced light culling schemes
as mentioned above but considering the limited amount of time I am
satisfied with the amount of featured I managed to implement.

As mentioned, the use of deferred shading is widespread in real time
rendering and the gaming industry. However due to the limitations
regarding for example transparent objects, there is a tendency to try to innovate
new algorithms that uses the best parts of both deferred and forward
rendering. Also, it is hard to predict what future graphics hardware
will bring in form of shader branching capabilities which could change
the situation.

# References
