
fxc /T cs_5_0 "win32/cca.compute.hlsl" /Fo "build/win32/cca.compute.cso" > $null
fxc /T vs_5_0 "win32/blit.vertex.hlsl" /Fo "build/win32/blit.vertex.cso" > $null
fxc /T ps_5_0 "win32/blit.pixel.hlsl" /Fo "build/win32/blit.pixel.cso" > $null
