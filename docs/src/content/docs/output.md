---
title: "Output"
description: "AutoAMP, WAV writing, wavtool assembly and the neural modes."
---

After synthesis, the note is normalized and written; the host then assembles the note buffers or hands the whole timeline to a neural mode.

`AutoAMP` scales every note by its own peak:

$$
g=\frac{0.5\,v_{\text{vol}}}{\max_n\left|y[n]\right|},
\qquad
y'[n]=\operatorname{clamp}\left(g\,y[n],-1,1\right).
$$

An all-zero note becomes silence. `WavIO::WriteWav` writes mono 16-bit PCM. The Python host then places the note buffers with `lesswavtool`; the placement, envelope and overlap equations are in [Rendering a UST](rendering.md). Whole-song polish and feature assembly replace the wavtool stage with one neural pass.

Back to [Processing pipeline](pipeline.md).
