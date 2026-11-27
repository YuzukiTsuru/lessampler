---
title: "Configuration configurator"
description: "Generate lessampler.less presets for common synthesis scenarios."
---

`scripts/lessampler_config.py` writes a complete `lessampler.less` for a named scenario. It does not download model weights; it fills model paths, warns when they are missing, and otherwise leaves the files alone.

```bash
python3 scripts/lessampler_config.py --list
python3 scripts/lessampler_config.py --scenario nhv-note \
  --models-dir ./models \
  --output ./lessampler.less
```

The output path defaults to `./lessampler.less`; an existing file is not overwritten unless `--force` is passed. `--dry-run` prints the TOML instead of writing it. Without `--scenario`, the script lists the scenarios and prompts for one. A binary built from a different commit logs `Configure file version does not match software version`; that is a warning, and the config still loads.

## Scenarios

| Id | Backend | F0 | Model | Assembly |
| --- | --- | --- | --- | --- |
| `world-baseline` | WORLD | Harvest | — | wavtool |
| `world-edit` | WORLD | Harvest | — | wavtool |
| `llsm-edit` | LLSM | LLSM refine | — | wavtool |
| `llsm-rmvpe` | LLSM | RMVPE | RMVPE | wavtool |
| `nhv-note` | neural | Harvest | NHVSing | wavtool |
| `pcnsf-note` | neural | Harvest | PC-NSF | wavtool |
| `nhv-note-rmvpe` | neural | RMVPE | NHVSing + RMVPE | wavtool |
| `pcnsf-note-rmvpe` | neural | RMVPE | PC-NSF + RMVPE | wavtool |
| `nhv-polish` | WORLD | Harvest | NHVSing | `--polish` |
| `pcnsf-polish` | WORLD | Harvest | PC-NSF | `--polish` |
| `nhv-feature` | WORLD | Harvest | NHVSing | `--feature` |
| `llsm-feature` | LLSM | LLSM refine | NHVSing | `--feature` |
| `pcnsf-feature` | WORLD | Harvest | PC-NSF | `--feature` |
| `nhv-source-mel` | neural | Harvest | NHVSing | experimental frontend |
| `pcnsf-source-mel` | neural | Harvest | PC-NSF | experimental frontend |

`llsm-rmvpe` and the `-rmvpe` neural presets need the RMVPE model in addition to the vocoder. The polish presets and `nhv-feature`/`pcnsf-feature` use a WORLD note backend; `llsm-feature` uses LLSM. All of them need a neural vocoder model for the final pass, and the script prints the corresponding `render_ust.py` flag.

## Model paths

`--models-dir` points to one directory containing the standard file names:

```text
nhv_v3_3.onnx
pc_nsf_hifigan_44.1k_hop512_128bin_2025.02.onnx
rmvpe.onnx
```

Use `--nhv-model`, `--pcnsf-model`, and `--rmvpe-model` for individual overrides. Paths are written as absolute paths by default; a relative path is resolved against the current working directory. The generated comments include the download URL, SHA-256, and license summary. See [DNN deployment](../guides/neural-deployment.md) for download commands and build setup.

If a path is omitted, the generated config contains `/absolute/path/to/...` and the script prints a warning. Replace it before rendering.

## Example

```bash
python3 scripts/lessampler_config.py \
  --scenario nhv-note \
  --nhv-model /opt/lessampler-models/nhv_v3_3.onnx \
  --output build/max-gcc/bin/lessampler.less \
  --force
```

For UTAU, the executable reads `lessampler.less` from its own directory. Generate into that directory, or copy the file there after editing. `<voicebank>/lessvoice.less` can override the global `[audio_model]`, `[f0]`, `[neural]`, and `[style]` values.

Use [Choosing a synthesis path](../processing/paths.md) to pick a scenario; the configurator only writes the configuration, not the UST rendering command.
