# Agent Guidelines for Brainfuck Interpreter

This document outlines the conventions and commands for agents operating within this Rust project.

## 1. Build/Lint/Test Commands

- **Build:** `cargo build`
- **Run tests:** `cargo test`
- **Run a single test:** `cargo test <test_name>`
- **Lint:** `cargo clippy`
- **Format:** `cargo fmt`

## 2. Code Style Guidelines

- **Imports:** Organize `use` statements alphabetically and group them (e.g., standard library, external crates, local modules).
- **Formatting:** Adhere to `rustfmt` conventions. Run `cargo fmt` to automatically format code.
- **Naming Conventions:**
    - `snake_case` for functions, variables, and modules.
    - `PascalCase` for types (structs, enums, traits).
    - `SCREAMING_SNAKE_CASE` for constants.
- **Types:** Use explicit types where clarity is improved; leverage type inference otherwise.
- **Error Handling:** Prefer `Result` and `Option` for recoverable errors. Use `panic!` sparingly for unrecoverable errors.
- **Comments:** Add comments for complex logic or explanations of *why* something is done, not *what*.
