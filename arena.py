import requests
import random
import time
import os
import sys
import re
import threading


# ============================================================
# CONFIGURATION
# ============================================================

OLLAMA_URL = "http://localhost:11434"

# Network / generation timeout
REQUEST_TIMEOUT = 240

# Retry failed generations
MAX_RETRIES = 1

# Maximum characters kept from a model response
MAX_RESPONSE_CHARS = 12000

# Generation settings
DEFAULT_TEMPERATURE = 0.2

# QoS:
# Keep generated responses reasonably short.
CONTESTANT_MAX_TOKENS = 768
JUDGE_MAX_TOKENS = 1024

# Context size
CONTEXT_SIZE = 4096

# Unload models after each generation.
# This is useful for GPUs with limited VRAM.
KEEP_ALIVE = "0"


# ============================================================
# TERMINAL COLORS
# ============================================================

def supports_color():
    if os.name == "nt":
        return True

    return sys.stdout.isatty()


USE_COLOR = supports_color()


def ansi(code):
    if USE_COLOR:
        return f"\033[{code}m"

    return ""


RESET = ansi("0")
BOLD = ansi("1")
DIM = ansi("2")
GREEN = ansi("32")
YELLOW = ansi("33")
BLUE = ansi("34")
CYAN = ansi("36")
RED = ansi("31")
WHITE = ansi("37")


# ============================================================
# TERMINAL UI
# ============================================================

def clear_screen():
    os.system("cls" if os.name == "nt" else "clear")


def terminal_width():
    try:
        return max(
            70,
            min(
                os.get_terminal_size().columns,
                120
            )
        )
    except OSError:
        return 90


def line(char="─"):
    print(char * terminal_width())


def title(text):
    width = terminal_width()
    inner = width - 4

    padding = max(
        0,
        (inner - len(text)) // 2
    )

    right_padding = max(
        0,
        inner - padding - len(text)
    )

    print()
    print(
        BOLD + CYAN +
        "╭" + "─" * (width - 2) + "╮" +
        RESET
    )

    print(
        BOLD + CYAN +
        "│ " +
        " " * padding +
        text +
        " " * right_padding +
        " │" +
        RESET
    )

    print(
        BOLD + CYAN +
        "╰" + "─" * (width - 2) + "╯" +
        RESET
    )

    print()


def section(text):
    print()
    print(
        BOLD +
        BLUE +
        text +
        RESET
    )
    line()


def status(label, message, color=WHITE):
    print(
        f"{color}[{label}]{RESET} {message}"
    )


def format_seconds(seconds):
    if seconds < 1:
        return f"{seconds * 1000:.0f} ms"

    if seconds < 60:
        return f"{seconds:.1f}s"

    minutes = int(seconds // 60)
    remaining = seconds % 60

    return f"{minutes}m {remaining:.1f}s"


# ============================================================
# SPINNER
# ============================================================

class Spinner:

    def __init__(self, message):
        self.message = message
        self.running = False
        self.thread = None
        self.start_time = None

    def start(self):
        self.running = True
        self.start_time = time.time()

        self.thread = threading.Thread(
            target=self._run,
            daemon=True
        )

        self.thread.start()

    def _run(self):
        frames = [
            "|",
            "/",
            "-",
            "\\"
        ]

        index = 0

        while self.running:

            elapsed = format_seconds(
                time.time() - self.start_time
            )

            text = (
                f"\r{CYAN}"
                f"{frames[index % len(frames)]}"
                f"{RESET} "
                f"{self.message} "
                f"{DIM}[{elapsed}]{RESET}"
            )

            print(
                text,
                end="",
                flush=True
            )

            index += 1

            time.sleep(0.12)

    def stop(
        self,
        final_message=None,
        success=True
    ):
        self.running = False

        if self.thread:
            self.thread.join(
                timeout=0.3
            )

        print(
            "\r" +
            " " * terminal_width() +
            "\r",
            end=""
        )

        if final_message:
            color = (
                GREEN
                if success
                else RED
            )

            print(
                f"{color}{final_message}{RESET}"
            )


# ============================================================
# OLLAMA CONNECTION
# ============================================================

def ollama_is_running():

    try:

        response = requests.get(
            f"{OLLAMA_URL}/api/tags",
            timeout=5
        )

        return response.status_code == 200

    except requests.RequestException:

        return False


# ============================================================
# MODEL DISCOVERY
# ============================================================

def get_installed_models():

    try:

        response = requests.get(
            f"{OLLAMA_URL}/api/tags",
            timeout=10
        )

        response.raise_for_status()

        data = response.json()

        models = []

        for item in data.get(
            "models",
            []
        ):

            name = item.get(
                "name"
            )

            if not name:
                continue

            model = {
                "name": name,
                "size": item.get(
                    "size",
                    0
                ),
                "modified_at": item.get(
                    "modified_at",
                    ""
                ),
                "parameter_size": "",
                "family": "",
                "quantization": ""
            }

            # Get detailed model metadata.
            try:

                detail = requests.post(
                    f"{OLLAMA_URL}/api/show",
                    json={
                        "name": name
                    },
                    timeout=10
                )

                if detail.ok:

                    info = detail.json()

                    details = info.get(
                        "details",
                        {}
                    )

                    model[
                        "parameter_size"
                    ] = details.get(
                        "parameter_size",
                        ""
                    )

                    model[
                        "family"
                    ] = details.get(
                        "family",
                        ""
                    )

                    model[
                        "quantization"
                    ] = details.get(
                        "quantization_level",
                        ""
                    )

            except requests.RequestException:
                pass

            models.append(model)

        return models

    except requests.RequestException as e:

        print()

        status(
            "ERROR",
            f"Cannot connect to Ollama: {e}",
            RED
        )

        return []


# ============================================================
# MODEL METADATA
# ============================================================

def parse_parameter_size(value):

    if not value:
        return 0.0

    value = str(
        value
    ).upper().strip()

    match = re.search(
        r"([0-9]+(?:\.[0-9]+)?)\s*([BM])",
        value
    )

    if not match:
        return 0.0

    number = float(
        match.group(1)
    )

    unit = match.group(2)

    if unit == "M":
        return number / 1000.0

    return number


def format_size(size):

    if not size:
        return "?"

    gb = size / (
        1024 ** 3
    )

    return f"{gb:.1f} GB"


def show_detected_models(models):

    section(
        "DETECTED OLLAMA MODELS"
    )

    if not models:

        status(
            "INFO",
            "No models installed.",
            YELLOW
        )

        return

    for index, model in enumerate(
        models,
        1
    ):

        print(
            f"{BOLD}{index}. "
            f"{model['name']}{RESET}"
        )

        print(
            f"   Parameters : "
            f"{model['parameter_size'] or '?'}"
        )

        print(
            f"   Size       : "
            f"{format_size(model['size'])}"
        )

        print(
            f"   Family     : "
            f"{model['family'] or '?'}"
        )

        print(
            f"   Quant      : "
            f"{model['quantization'] or '?'}"
        )

        print()


# ============================================================
# AUTOMATIC ROLE SELECTION
# ============================================================

def calculate_judge_score(model):

    score = 0.0

    params = parse_parameter_size(
        model.get(
            "parameter_size",
            ""
        )
    )

    size_gb = 0.0

    if model.get("size"):

        size_gb = (
            model["size"]
            /
            (1024 ** 3)
        )

    # Larger models generally provide
    # stronger evaluation capability.
    score += params * 100

    # File size is another weak signal.
    score += size_gb * 2

    text = (
        str(
            model.get(
                "family",
                ""
            )
        )
        + " "
        +
        str(
            model.get(
                "name",
                ""
            )
        )
    ).lower()

    instruction_terms = [
        "instruct",
        "instruction",
        "chat"
    ]

    for term in instruction_terms:

        if term in text:
            score += 20

    return score


def automatically_assign_roles(
    models
):

    # One model:
    # use it directly.
    if len(models) == 1:

        return [], models[0]

    ranked = sorted(
        models,
        key=calculate_judge_score,
        reverse=True
    )

    judge = ranked[0]

    contestants = ranked[1:]

    return contestants, judge


# ============================================================
# QOS
# ============================================================

def model_qos(
    model,
    role="contestant"
):

    params = parse_parameter_size(
        model.get(
            "parameter_size",
            ""
        )
    )

    if role == "judge":

        max_tokens = (
            JUDGE_MAX_TOKENS
        )

    elif role == "planner":

        # Input planner needs very little
        # output, so keep it extremely small.
        max_tokens = 256

    else:

        max_tokens = (
            CONTESTANT_MAX_TOKENS
        )

    if params >= 20:

        context = 3072

    elif params >= 10:

        context = 3584

    else:

        context = CONTEXT_SIZE

    return {
        "num_ctx": context,
        "num_predict": max_tokens,
        "temperature": DEFAULT_TEMPERATURE
    }


# ============================================================
# GENERIC MODEL REQUEST
# ============================================================

def ask_model(
    model,
    prompt,
    role="contestant"
):

    settings = model_qos(
        model,
        role
    )

    payload = {
        "model": model["name"],
        "prompt": prompt,
        "stream": False,
        "keep_alive": KEEP_ALIVE,
        "options": {
            "temperature":
                settings["temperature"],

            "num_ctx":
                settings["num_ctx"],

            "num_predict":
                settings["num_predict"]
        }
    }

    last_error = None

    for attempt in range(
        MAX_RETRIES + 1
    ):

        spinner = Spinner(
            f"{role.capitalize()} running: "
            f"{model['name']}"
        )

        spinner.start()

        start = time.time()

        try:

            response = requests.post(
                f"{OLLAMA_URL}/api/generate",
                json=payload,
                timeout=REQUEST_TIMEOUT
            )

            elapsed = (
                time.time() - start
            )

            response.raise_for_status()

            data = response.json()

            answer = data.get(
                "response",
                ""
            ).strip()

            spinner.stop(
                f"Done {model['name']} "
                f"({format_seconds(elapsed)})",
                success=True
            )

            if not answer:

                raise RuntimeError(
                    "Model returned an empty response."
                )

            if len(answer) > MAX_RESPONSE_CHARS:

                answer = (
                    answer[
                        :MAX_RESPONSE_CHARS
                    ].rstrip()
                    +
                    "\n\n"
                    "[Response truncated by arena QoS limit.]"
                )

            return (
                answer,
                elapsed,
                None
            )

        except Exception as e:

            elapsed = (
                time.time() - start
            )

            last_error = str(e)

            spinner.stop(
                f"Failed: {model['name']} "
                f"({format_seconds(elapsed)})",
                success=False
            )

            if attempt < MAX_RETRIES:

                status(
                    "RETRY",
                    f"Retrying {model['name']}...",
                    YELLOW
                )

                time.sleep(0.5)

    return (
        None,
        0,
        last_error
    )


# ============================================================
# CONTESTANT PROMPT
# ============================================================

def create_contestant_prompt(
    question
):

    return f"""
Answer the user's question directly.

Give an accurate, useful and self-contained answer.

Use the information provided by the user.

Do not mention:
- this arena
- other models
- contestants
- judges
- scoring
- evaluation

User question:

{question}

Answer:
""".strip()


# ============================================================
# INPUT REQUIREMENT PLANNER
# ============================================================

def create_input_planner_prompt(
    question
):

    return f"""
You are an input requirement detector.

Determine whether the user's question can be answered properly
using only the information already provided.

Do NOT ask for optional preferences if the question can already
be answered reasonably.

Ask for additional information ONLY when missing information is
necessary to give a useful answer.

Examples:

Question:
"What is binary search?"
Answer:
NEED_INPUT: NO

Question:
"Calculate my BMI"
Answer:
NEED_INPUT: YES
QUESTION_1: What is your height?
QUESTION_2: What is your weight?

Question:
"Debug this code"
Answer:
NEED_INPUT: YES
QUESTION_1: Please provide the code you want debugged.

Question:
"Write a Python program to sort an array"
Answer:
NEED_INPUT: NO

Question:
"Convert this temperature to Fahrenheit"
Answer:
NEED_INPUT: YES
QUESTION_1: What temperature should be converted?

Important:
- Do not solve the user's question.
- Do not provide explanations.
- Do not ask unnecessary questions.
- Ask only for information that is genuinely required.
- Maximum 5 questions.

Use EXACTLY this format:

NEED_INPUT: YES

QUESTION_1: ...
QUESTION_2: ...
QUESTION_3: ...
QUESTION_4: ...
QUESTION_5: ...

OR:

NEED_INPUT: NO

User question:

{question}
""".strip()


def parse_input_plan(
    output
):

    if not output:
        return False, []

    need_input = False
    questions = []

    for raw_line in output.splitlines():

        line = raw_line.strip()

        upper = line.upper()

        if upper.startswith(
            "NEED_INPUT:"
        ):

            value = (
                line.split(
                    ":",
                    1
                )[1]
                .strip()
                .upper()
            )

            need_input = (
                value.startswith("YES")
            )

        match = re.match(
            r"^QUESTION_\d+\s*:\s*(.+)$",
            line,
            re.IGNORECASE
        )

        if match:

            question = (
                match.group(1)
                .strip()
            )

            if question:
                questions.append(
                    question
                )

    # If the planner says yes but somehow
    # produced no questions, don't trap
    # the user in an empty input step.
    if need_input and not questions:
        need_input = False

    return (
        need_input,
        questions[:5]
    )


# ============================================================
# AUTOMATIC USER INPUT COLLECTION
# ============================================================

def check_for_required_input(
    question,
    planner
):

    prompt = create_input_planner_prompt(
        question
    )

    answer, elapsed, error = ask_model(
        planner,
        prompt,
        role="input check"
    )

    if error:

        # If the planner fails, don't block
        # the arena. Just proceed.
        status(
            "INFO",
            "Input check unavailable; "
            "continuing with the supplied question.",
            YELLOW
        )

        return question

    need_input, questions = (
        parse_input_plan(answer)
    )

    if not need_input:

        return question

    section(
        "ADDITIONAL INFORMATION"
    )

    answers = []

    for index, required_question in enumerate(
        questions,
        1
    ):

        print(
            f"{BOLD}{index}. "
            f"{required_question}{RESET}"
        )

        while True:

            try:

                user_answer = input(
                    "   > "
                ).strip()

            except KeyboardInterrupt:

                print()

                status(
                    "EXIT",
                    "Cancelled.",
                    YELLOW
                )

                raise SystemExit

            if user_answer:

                answers.append(
                    user_answer
                )

                break

            status(
                "INFO",
                "Please provide an answer.",
                YELLOW
            )

    # Add collected information to the
    # original question.
    enriched_question = (
        question
        +
        "\n\nAdditional information "
        "provided by the user:\n"
    )

    for index, answer in enumerate(
        answers,
        1
    ):

        enriched_question += (
            f"{index}. {answer}\n"
        )

    return enriched_question.strip()


# ============================================================
# LABEL GENERATION
# ============================================================

def generate_labels(
    count
):

    labels = []

    for index in range(
        count
    ):

        if index < 26:

            labels.append(
                chr(
                    ord("A") +
                    index
                )
            )

        else:

            labels.append(
                f"R{index + 1}"
            )

    return labels


# ============================================================
# ANONYMOUS RESPONSES
# ============================================================

def anonymize_responses(
    responses
):

    shuffled = responses[:]

    random.shuffle(
        shuffled
    )

    labels = generate_labels(
        len(shuffled)
    )

    anonymous = []

    for label, item in zip(
        labels,
        shuffled
    ):

        anonymous.append(
            (
                label,
                item["answer"]
            )
        )

    return anonymous


# ============================================================
# JUDGE PROMPT
# ============================================================

def create_judge_prompt(
    question,
    responses
):

    blocks = []

    for label, answer in responses:

        blocks.append(
            f"""
===== RESPONSE {label} =====

{answer}

===== END RESPONSE {label} =====
""".strip()
        )

    joined = "\n\n".join(
        blocks
    )

    return f"""
You are the final evaluator and answer synthesizer.

The user's question is:

{question}

Below are anonymous answers from different language models.

{joined}

Evaluate EVERY response.

For each response assign:

Correctness: 0-10
Relevance: 0-10
Reasoning: 0-10
Completeness: 0-10
Clarity: 0-10

Then select the strongest response.

Finally, create a NEW high-quality answer to the user's
original question.

The final answer must:
- directly answer the question
- use useful information from the responses
- correct obvious mistakes
- be self-contained
- not mention this evaluation
- not mention models
- not mention contestants
- not mention scores
- not mention response labels

Use EXACTLY this format:

WINNER: A

A_CORRECTNESS: 0
A_RELEVANCE: 0
A_REASONING: 0
A_COMPLETENESS: 0
A_CLARITY: 0
A_TOTAL: 0

B_CORRECTNESS: 0
B_RELEVANCE: 0
B_REASONING: 0
B_COMPLETENESS: 0
B_CLARITY: 0
B_TOTAL: 0

Continue for every response.

REASON:
brief explanation

FINAL_ANSWER:
write the complete final answer here
END_FINAL_ANSWER

Do not use JSON.

Do not put anything after END_FINAL_ANSWER.
""".strip()


# ============================================================
# JUDGE OUTPUT PARSER
# ============================================================

def parse_integer(
    value,
    maximum
):

    try:

        match = re.search(
            r"-?\d+",
            str(value)
        )

        if not match:
            return 0

        number = int(
            match.group()
        )

        return max(
            0,
            min(
                number,
                maximum
            )
        )

    except (
        AttributeError,
        ValueError
    ):

        return 0


def parse_judge_output(
    output,
    labels
):

    scores = {}

    winner = None
    reason = ""
    final_answer = ""

    in_reason = False
    in_final = False

    if not output:

        return {
            "winner": None,
            "scores": {},
            "reason": "",
            "final_answer": ""
        }

    for raw_line in output.splitlines():

        line = raw_line.strip()

        upper = line.upper()

        if upper.startswith(
            "WINNER:"
        ):

            value = (
                line.split(
                    ":",
                    1
                )[1]
                .strip()
                .upper()
            )

            if value in labels:
                winner = value

            in_reason = False
            in_final = False

            continue

        if upper == "REASON:":

            in_reason = True
            in_final = False

            continue

        if upper == "FINAL_ANSWER:":

            in_final = True
            in_reason = False

            continue

        if upper == "END_FINAL_ANSWER":

            in_final = False

            continue

        if in_final:

            final_answer += (
                raw_line +
                "\n"
            )

            continue

        if in_reason:

            reason += (
                raw_line +
                "\n"
            )

            continue

        match = re.match(
            r"^([A-Za-z0-9]+)_(CORRECTNESS|RELEVANCE|REASONING|COMPLETENESS|CLARITY|TOTAL):\s*(.*)$",
            line,
            re.IGNORECASE
        )

        if match:

            label = (
                match.group(1)
                .upper()
            )

            category = (
                match.group(2)
                .upper()
            )

            value = match.group(3)

            if label not in scores:

                scores[label] = {}

            if category == "TOTAL":

                scores[label][
                    "TOTAL"
                ] = parse_integer(
                    value,
                    50
                )

            else:

                scores[label][
                    category
                ] = parse_integer(
                    value,
                    10
                )

    # Calculate totals if the judge
    # omitted them.
    for label in labels:

        if label not in scores:

            scores[label] = {}

        if "TOTAL" not in scores[label]:

            categories = [
                "CORRECTNESS",
                "RELEVANCE",
                "REASONING",
                "COMPLETENESS",
                "CLARITY"
            ]

            scores[label][
                "TOTAL"
            ] = sum(
                scores[label].get(
                    category,
                    0
                )
                for category in categories
            )

    # Fallback winner.
    if winner not in labels:

        winner = max(
            labels,
            key=lambda x:
            scores[x].get(
                "TOTAL",
                0
            )
        )

    return {
        "winner": winner,
        "scores": scores,
        "reason": reason.strip(),
        "final_answer":
            final_answer.strip()
    }


# ============================================================
# DISPLAY ROLES
# ============================================================

def show_roles(
    contestants,
    judge,
    planner
):

    section(
        "AUTOMATIC ARENA CONFIGURATION"
    )

    print(
        f"{BOLD}Input planner{RESET}"
    )

    print(
        f"  {planner['name']}"
    )

    print()

    print(
        f"{BOLD}Judge{RESET}"
    )

    print(
        f"  {judge['name']} "
        f"({judge.get('parameter_size') or '?'})"
    )

    print()

    print(
        f"{BOLD}Contestants{RESET}"
    )

    if not contestants:

        print(
            "  None"
        )

    else:

        for model in contestants:

            print(
                f"  - {model['name']} "
                f"({model.get('parameter_size') or '?'})"
            )

    print()


# ============================================================
# RESPONSE PREVIEW
# ============================================================

def show_compact_response(
    model,
    answer,
    elapsed
):

    print()

    print(
        f"{BOLD}{model['name']}{RESET} "
        f"{DIM}"
        f"({format_seconds(elapsed)})"
        f"{RESET}"
    )

    preview = answer.strip()

    if len(preview) > 500:

        preview = (
            preview[:500]
            .rstrip()
            +
            "..."
        )

    print(
        preview
    )


# ============================================================
# RESULTS
# ============================================================

def show_results(
    result,
    anonymous_responses
):

    section(
        "ARENA RESULTS"
    )

    scores = result[
        "scores"
    ]

    winner = result[
        "winner"
    ]

    for label, answer in anonymous_responses:

        data = scores.get(
            label,
            {}
        )

        total = data.get(
            "TOTAL",
            0
        )

        if label == winner:

            marker = (
                GREEN +
                "*" +
                RESET
            )

        else:

            marker = " "

        print(
            f"{marker} Response {label}: "
            f"{BOLD}{total}/50{RESET}"
        )

        print(
            f"    Correctness : "
            f"{data.get('CORRECTNESS', 0)}/10"
        )

        print(
            f"    Relevance   : "
            f"{data.get('RELEVANCE', 0)}/10"
        )

        print(
            f"    Reasoning   : "
            f"{data.get('REASONING', 0)}/10"
        )

        print(
            f"    Completeness: "
            f"{data.get('COMPLETENESS', 0)}/10"
        )

        print(
            f"    Clarity     : "
            f"{data.get('CLARITY', 0)}/10"
        )

        print()


# ============================================================
# FINAL ANSWER
# ============================================================

def show_final_answer(
    answer
):

    section(
        "FINAL ANSWER"
    )

    print(
        answer.strip()
    )

    print()


# ============================================================
# SINGLE MODEL MODE
# ============================================================

def run_single_model(
    question,
    model
):

    section(
        "SINGLE MODEL MODE"
    )

    prompt = create_contestant_prompt(
        question
    )

    answer, elapsed, error = ask_model(
        model,
        prompt,
        role="answer"
    )

    if error:

        status(
            "ERROR",
            error,
            RED
        )

        return

    show_final_answer(
        answer
    )

    status(
        "TIME",
        f"{model['name']} responded in "
        f"{format_seconds(elapsed)}",
        DIM
    )


# ============================================================
# FULL ARENA
# ============================================================

def run_arena(
    question,
    models,
    planner,
    judge
):

    contestants, _ = (
        automatically_assign_roles(
            models
        )
    )

    # --------------------------------------------------------
    # Single model
    # --------------------------------------------------------

    if not contestants:

        run_single_model(
            question,
            judge
        )

        return

    # --------------------------------------------------------
    # Input requirement check
    # --------------------------------------------------------

    question = check_for_required_input(
        question,
        planner
    )

    # --------------------------------------------------------
    # Contestants
    # --------------------------------------------------------

    section(
        "CONTESTANT GENERATION"
    )

    responses = []

    prompt = create_contestant_prompt(
        question
    )

    for index, model in enumerate(
        contestants,
        1
    ):

        print(
            f"{BOLD}"
            f"[{index}/{len(contestants)}]"
            f"{RESET} "
            f"Running contestant..."
        )

        answer, elapsed, error = ask_model(
            model,
            prompt,
            role="contestant"
        )

        if error:

            status(
                "SKIP",
                f"{model['name']}: {error}",
                RED
            )

            continue

        responses.append({
            "model":
                model["name"],

            "answer":
                answer,

            "time":
                elapsed
        })

        show_compact_response(
            model,
            answer,
            elapsed
        )

    if not responses:

        status(
            "ERROR",
            "All contestant models failed.",
            RED
        )

        return

    # --------------------------------------------------------
    # Anonymous responses
    # --------------------------------------------------------

    anonymous = anonymize_responses(
        responses
    )

    print()

    status(
        "INFO",
        f"{len(anonymous)} "
        "contestant responses ready for judging.",
        CYAN
    )

    # --------------------------------------------------------
    # Judge
    # --------------------------------------------------------

    section(
        "JUDGING"
    )

    judge_prompt = create_judge_prompt(
        question,
        anonymous
    )

    judge_output, judge_time, judge_error = ask_model(
        judge,
        judge_prompt,
        role="judge"
    )

    if judge_error:

        status(
            "ERROR",
            f"Judge failed: {judge_error}",
            RED
        )

        # Safe fallback.
        fallback = (
            responses[0]["answer"]
        )

        show_final_answer(
            fallback
        )

        status(
            "FALLBACK",
            "Using first valid contestant response.",
            YELLOW
        )

        return

    # --------------------------------------------------------
    # Parse judge
    # --------------------------------------------------------

    labels = [
        label
        for label, _ in anonymous
    ]

    result = parse_judge_output(
        judge_output,
        labels
    )

    # --------------------------------------------------------
    # Final answer
    # --------------------------------------------------------

    final_answer = result[
        "final_answer"
    ]

    # If the judge failed to create
    # a final answer, use its winner.
    if not final_answer:

        winner = result[
            "winner"
        ]

        for label, answer in anonymous:

            if label == winner:

                final_answer = answer
                break

    # Absolute fallback.
    if not final_answer:

        final_answer = (
            responses[0]["answer"]
        )

    # --------------------------------------------------------
    # Results
    # --------------------------------------------------------

    show_results(
        result,
        anonymous
    )

    if result["reason"]:

        section(
            "JUDGE SUMMARY"
        )

        print(
            result["reason"]
        )

    show_final_answer(
        final_answer
    )

    status(
        "DONE",
        f"Judge completed in "
        f"{format_seconds(judge_time)}",
        GREEN
    )


# ============================================================
# MAIN
# ============================================================

def main():

    clear_screen()

    title(
        "ARENAFIGHT AI"
    )

    print(
        f"{DIM}"
        "Local multi-model LLM arena "
        "powered by Ollama"
        f"{RESET}"
    )

    print()

    # --------------------------------------------------------
    # Check Ollama
    # --------------------------------------------------------

    spinner = Spinner(
        "Connecting to Ollama"
    )

    spinner.start()

    start = time.time()

    running = ollama_is_running()

    elapsed = (
        time.time() - start
    )

    if running:

        spinner.stop(
            "Connected to Ollama "
            f"({format_seconds(elapsed)})",
            success=True
        )

    else:

        spinner.stop(
            "Ollama is not running.",
            success=False
        )

        print()

        status(
            "FIX",
            "Start Ollama and run the program again.",
            YELLOW
        )

        return

    # --------------------------------------------------------
    # Detect models
    # --------------------------------------------------------

    models = get_installed_models()

    if not models:

        print()

        status(
            "INFO",
            "No Ollama models were found.",
            YELLOW
        )

        print()

        print(
            "Install a model using:"
        )

        print()

        print(
            "    ollama pull <model>"
        )

        return

    show_detected_models(
        models
    )

    # --------------------------------------------------------
    # Automatic role assignment
    # --------------------------------------------------------

    contestants, judge = (
        automatically_assign_roles(
            models
        )
    )

    # The planner is automatically selected
    # as the most suitable available model.
    #
    # Prefer the judge because it is generally
    # the strongest model, but it runs separately
    # and is unloaded after each call.
    planner = judge

    show_roles(
        contestants,
        judge,
        planner
    )

    # --------------------------------------------------------
    # User question
    # --------------------------------------------------------

    print()

    try:

        question = input(
            f"{BOLD}{CYAN}"
            "Ask your question:"
            f"{RESET} "
        ).strip()

    except KeyboardInterrupt:

        print()

        print()

        status(
            "EXIT",
            "Cancelled.",
            YELLOW
        )

        return

    if not question:

        status(
            "INFO",
            "No question entered.",
            YELLOW
        )

        return

    print()

    # --------------------------------------------------------
    # Run
    # --------------------------------------------------------

    total_start = time.time()

    run_arena(
        question,
        models,
        planner,
        judge
    )

    total_elapsed = (
        time.time() -
        total_start
    )

    print()

    line()

    status(
        "TOTAL",
        format_seconds(
            total_elapsed
        ),
        DIM
    )

    print()


# ============================================================
# ENTRY POINT
# ============================================================

if __name__ == "__main__":
    main()