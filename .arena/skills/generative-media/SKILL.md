---
name: generative-media
description: Multimodal neural media pipeline for Fooocus SDXL offline image generation, Wan AI 2.1 video generation, ComfyUI workflows, local RTX 4060 8GB VRAM budgeting, and Level 1 Physical Evidence magic bytes validation.
---

# Generative Media Engineering Skill

## Overview
This skill guides autonomous agents in formulating, executing, and physically verifying multimodal media generation tasks across Fooocus SDXL, Wan AI 2.1, and ComfyUI architectures.

## Hardware Constraints and VRAM Budgeting
Host machine hardware specification: NVIDIA GeForce RTX 4060 Laptop GPU (8.00 GB VRAM, Driver 592.82, CUDA active).

1. **Fooocus SDXL Image Generation**:
   - Model: SDXL Base 1.0 with Fooocus Speed / Quality presets.
   - VRAM Footprint: ~4.5 GB to 6.2 GB VRAM.
   - Execution: Fully local, offline execution. Generates 1024x1024 visual assets in 12-25 seconds.
   - Endpoint: `http://127.0.0.1:8888` (default Fooocus webui/api daemon).

2. **Wan AI 2.1 Video Generation**:
   - Model variants:
     - Wan 2.1 1.3B (Local): Requires FP8 or 4-bit quantization with CPU offloading to fit in 8 GB VRAM. Optimal resolution: 832x480, 81 frames (~5 seconds @ 16 fps).
     - Wan 2.1 14B (Remote / API): Offloaded to remote inference server or cloud endpoint due to 24 GB+ VRAM requirement.
   - Modes:
     - Text-to-Video (T2V): Synthesize motion dynamics directly from structured prompt text.
     - Image-to-Video (I2V): Animate a pre-generated Fooocus SDXL keyframe with consistent characters and motion vectors.
   - Endpoint: `http://127.0.0.1:7860` (Wan 2.1 API daemon or ComfyUI Video).

## Prompt Engineering Doctrine
1. **Positive Prompt Structure**:
   - Subject definition: Primary subject, attire, expressions, anatomical precision.
   - Environment and lighting: Volumetric lighting, atmospheric perspective, cinematic rim lighting.
   - Technical rendering parameters: High-resolution detail, sharp focus, 8k render, octane render style.
2. **Negative Prompting Constraints**:
   - Standard filters: `blurry, deformed, bad anatomy, duplicate limbs, watermarks, text, oversaturated, low quality`.
3. **Motion Conditioning (Video)**:
   - Explicitly specify camera motion: `slow pan right`, `tracking shot`, `steady static camera`, `dolly zoom`.
   - Explicitly specify temporal physics: fluid motion, wind disturbance, natural human kinetics.

## Level 1 Physical Evidence Verification
In accordance with the Level 1 Physical Dominance Rule, generated media is never marked complete based on model consensus or completion strings alone. Every generated media asset must pass 4 Physical Evidence Gates:
1. **Gate 1 - Physical Presence**: File exists on disk at the declared path with byte size > 0.
2. **Gate 2 - Magic Bytes Header Verification**:
   - PNG files: Header starts with `89 50 4E 47 0D 0A 1A 0A`.
   - JPEG files: Header starts with `FF D8 FF`.
   - MP4 containers: Header contains ISO BMFF `ftyp` box within the first 16 bytes.
3. **Gate 3 - Metadata and Resolution**:
   - Parse image width and height or video frame count, fps, and duration.
4. **Gate 4 - Cryptographic Audit Registration**:
   - Compute SHA-256 hash of the generated asset and record it in `.arena/security_audit.jsonl` via `SecuritySandbox::logAuditEvent`.

