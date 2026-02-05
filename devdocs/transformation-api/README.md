# Transformation API Learning Materials

This directory contains educational materials designed to help you deeply understand, critique, and contribute to the llvm-nanobind transformation API work.

## The Philosophy

> "Human review as the bottleneck for AI work" suggests an education problem.

These materials aim to make you capable of:
1. **Understanding** what was built and why
2. **Critiquing** the design decisions made
3. **Contributing** better ideas for what comes next

The goal isn't just verifying a diff—it's growing your expertise.

---

## Learning Path

### Quick Start (30 min)
If you're short on time:
1. Read **cheat-sheet.md** for API reference
2. Skim **architecture-diagrams.md** for visual overview
3. Jump to Part 4 of **learning-guide.md** for critique framework

### Full Journey (2-3 hours)
For deep understanding:

```
┌─────────────────────────────────────────────────────────────┐
│                    START HERE                               │
│                                                             │
│  1. learning-guide.md (Part 1-3)                           │
│     - Foundational concepts                                 │
│     - API design                                            │
│     - Obfuscation case studies                             │
│                         │                                   │
│                         ▼                                   │
│  2. hands-on-exercise.py                                   │
│     - Interactive coding exercises                          │
│     - Build intuition through practice                      │
│     uv run devdocs/transformation-api/hands-on-exercise.py │
│                         │                                   │
│                         ▼                                   │
│  3. interactive-quiz.py                                    │
│     - Test your understanding                               │
│     - Identify gaps                                         │
│     uv run devdocs/transformation-api/interactive-quiz.py  │
│                         │                                   │
│                         ▼                                   │
│  4. learning-guide.md (Part 4-6)                           │
│     - Critical evaluation framework                         │
│     - Exercises and self-assessment                         │
│     - Synthesis questions                                   │
│                         │                                   │
│                         ▼                                   │
│  5. Study the actual code                                  │
│     - tools/obfuscation/*.py                               │
│     - src/llvm-nanobind.cpp                                │
│                                                             │
│                    YOU'RE READY                             │
│           to review, critique, and contribute               │
└─────────────────────────────────────────────────────────────┘
```

---

## Materials Index

### Core Learning

| File | Purpose | Format |
|------|---------|--------|
| **learning-guide.md** | Comprehensive tutorial with quizzes and exercises | Text (book-like) |
| **hands-on-exercise.py** | Interactive coding walkthrough | Executable script |
| **interactive-quiz.py** | Self-assessment with explanations | Executable script |

### Reference

| File | Purpose | Format |
|------|---------|--------|
| **cheat-sheet.md** | Quick API reference | Text (table) |
| **architecture-diagrams.md** | Visual system overview | ASCII diagrams |
| **plan.md** | Improvement roadmap (priorities) | Text |
| **progress.md** | Current status | Text |

### Background

| File | Purpose |
|------|---------|
| **../porting-guide.md** | Detailed API differences from C++ |
| **../../tools/obfuscation/*.py** | Working transformation examples |
| **../../README.md** | Project overview |

---

## How to Use These Materials

### For Understanding
Read the learning guide progressively. Don't skip the exercises.

### For Coding
Keep the cheat-sheet open. Reference architecture diagrams when confused.

### For Reviewing
Use Part 4 of learning-guide.md as a critique framework. Apply the lenses.

### For Contributing
Study plan.md for priorities. Understand *why* before *how*.

---

## Prerequisites

These materials assume:
- Basic programming in Python
- Familiarity with compilers (at a high level)
- No prior LLVM experience required

If you need a refresher:
- [LLVM Language Reference](https://llvm.org/docs/LangRef.html) - The IR spec
- [Compilers: Principles, Techniques, and Tools](https://en.wikipedia.org/wiki/Compilers:_Principles,_Techniques,_and_Tools) - The Dragon Book

---

## Feedback

These materials were created to support human review of AI-assisted work. If you find:
- Concepts that are confusing
- Missing explanations
- Incorrect information
- Better pedagogical approaches

Please improve them. The goal is building shared understanding, not preserving a document.

---

*"The best time to understand is before you need to critique. The second best time is now."*
