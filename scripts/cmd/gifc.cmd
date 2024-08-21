::create a gif from video
::args given: (1) video filename (2) scale (3) output name

::stop bragging
::@echo off

::do real shit
ffmpeg -y -i %1 -filter_complex "fps=20,scale=%2:-1:flags=lanczos,split[s0][s1];[s0]palettegen=max_colors=32[p];[s1][p]paletteuse=dither=bayer" %3.gif