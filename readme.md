# ArenaFight AI

ArenaFight AI is a local multi-model Large Language Model (LLM) arena powered by Ollama. It allows users to query multiple locally installed models simultaneously, anonymously evaluates their responses through a designated judge model, and synthesizes a final high-quality answer.

---

## Features

* **Local Multi-Model Evaluation**: Automatically runs a user prompt across multiple local LLMs and presents anonymous responses for objective comparison.


* **Automated Role Assignment**: Dynamically assigns roles (such as judge and contestants) based on model parameter sizes and metadata.


* **Input Requirement Planner**: Detects whether additional information is required before answering and prompts the user interactively if necessary.


* **Structured Judging System**: Evaluates each contestant model across multiple criteria including correctness, relevance, reasoning, completeness, and clarity.


* **Robust Terminal Interface**: Features color-coded output, custom spinners, execution time tracking, and automatic retry mechanisms for failed generations.



---

## Requirements

* Python 3.x


* `requests` Python library


* [Ollama](https://ollama.com/) running locally on `http://localhost:11434`


---

## Installation & Setup

1. Ensure Ollama is installed and running on your system:
```bash
ollama serve

```


2. Install the required Python dependencies:
```bash
pip install requests

```


3. Download at least one model via Ollama (two or more models are recommended to utilize the arena feature):
```bash
ollama pull <model_name>

```



---

## Usage

Run the script from your terminal:

```bash
python arena.py

```

The program will automatically connect to Ollama, scan your installed models, assign evaluation roles, prompt you for a question, and display the arena battle results followed by the synthesized final answer.
