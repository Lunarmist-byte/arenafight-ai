# Multimodal Neural Media Pipeline: Wan AI (Wan2.1) + Fooocus + Ollama

**Architecture Specification & Integration Guide**  
**Author:** Lunarmist-byte  
**GitHub:** [https://github.com/Lunarmist-byte](https://github.com/Lunarmist-byte)  
**LinkedIn:** [https://www.linkedin.com/in/amal-s-kumar-ba69a1290/](https://www.linkedin.com/in/amal-s-kumar-ba69a1290/)  

---

## Quick Navigation
- Full Technical Architecture & Guide: [`docs/WAN_AI_FOOOCUS_OLLAMA_INTEGRATION.md`](file:///c:/Users/amals/Documents/Codeworks/Arenafight-ai/docs/WAN_AI_FOOOCUS_OLLAMA_INTEGRATION.md)
- Production Automation Script: [`scripts/ollama_wan_fooocus_pipeline.py`](file:///c:/Users/amals/Documents/Codeworks/Arenafight-ai/scripts/ollama_wan_fooocus_pipeline.py)
- Generative Media Skill Specification: [`.arena/skills/generative-media/SKILL.md`](file:///c:/Users/amals/Documents/Codeworks/Arenafight-ai/.arena/skills/generative-media/SKILL.md)
- ArenaFight Native C++ Media Tools: [`include/arenafight/tools/media_tools.hpp`](file:///c:/Users/amals/Documents/Codeworks/Arenafight-ai/include/arenafight/tools/media_tools.hpp)

---

## 1. How the Pipeline Works

```text
+-----------------------+     Structured JSON      +-----------------------+     1024x1024 PNG     +-----------------------+
|        OLLAMA         | -----------------------> |        FOOOCUS        | --------------------> |       WAN AI 2.1      |
|  (Cognitive Director) |   (Prompts & Motion)     |    (SDXL Keyframe)    |   (Visual Keyframe)   |    (Video Dynamics)   |
+-----------------------+                          +-----------------------+                       +-----------------------+
           |                                                   |                                                   |
           v                                                   v                                                   v
Expands creative ideas into                      Generates high-resolution                       Synthesizes 81-frame cinematic
detailed scene aesthetics,                       photorealistic still visual                     video sequence (16 fps) with
lighting, and camera kinetics                    with verified magic bytes                       verified MP4 ISO BMFF container
```

### Roles in the Architecture
1. **Ollama / Local LLM (Cognitive Director):**
   - Translates sparse user prompts into rich visual descriptors (lighting, textures, lens optics, aspect ratio, negative filters).
   - Formulates temporal camera motion vectors (`slow dolly-in`, `tracking pan`, `aerial crane shot`) for Wan AI.
2. **Fooocus (Neural Illustrator):**
   - Offline SDXL engine generating high-resolution keyframe images (`.png`).
   - Runs headless on port `8888` via `--enable-apis`.
3. **Wan AI 2.1 (Neural Cinematographer):**
   - Video Diffusion Transformer (DiT) from Alibaba WanX.
   - Runs in Image-to-Video (I2V) mode on port `7860`, animating the Fooocus keyframe into an 81-frame MP4 video (~5 seconds at 16 fps).

---

## 2. VRAM Strategy on 8GB GPUs (e.g. NVIDIA RTX 4060)

Running all three neural models simultaneously will exceed 8GB VRAM and trigger an Out Of Memory (OOM) error. The system solves this with **Sequential Stage Execution**:

| Pipeline Stage | Active Model | VRAM Used | Action on Completion |
| :--- | :--- | :---: | :--- |
| **Stage 1: Cognition** | Ollama (`qwen2.5-coder` / `deepseek-r1`) | ~4.8 GB | Outputs scene JSON, parks / releases memory |
| **Stage 2: Keyframe** | Fooocus SDXL Base 1.0 | ~5.5 GB | Writes verified PNG to disk, purges latent cache |
| **Stage 3: Video Motion** | Wan AI 2.1 (1.3B FP8 Quantized) | ~7.2 GB | Animates keyframe into MP4 container |

---

## 3. Quickstart

### 1. Run Autonomous Python Script
```bash
python scripts/ollama_wan_fooocus_pipeline.py "futuristic cyberpunk racing car in neon rain"
```

### 2. Run via ArenaFight Native CLI
```bash
arena "Generate a concept keyframe with Fooocus and animate a video clip with Wan AI for an orbital space station"
```

### 3. Verify Output Assets via Level 1 Physical Gates
```bash
arena run "media_inspect path=generated_media/scene_keyframe.png"
arena run "media_inspect path=generated_media/scene_cinematic.mp4"
```

---

Made by Lunarmist-byte  
GitHub: [https://github.com/Lunarmist-byte](https://github.com/Lunarmist-byte)  
LinkedIn: [https://www.linkedin.com/in/amal-s-kumar-ba69a1290/](https://www.linkedin.com/in/amal-s-kumar-ba69a1290/)

