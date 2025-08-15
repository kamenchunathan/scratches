use std::{fs::OpenOptions, io::Read};

use anyhow::{self, Context};
use biffer::{instruction::tokenize, interpreter::basic::BasicInterpreter};
use clap::Parser;

/// Biffer is an optimizing brainfuck compiler and interpreter
#[derive(Parser)]
struct BifferArgs {
    /// Brainfuck program to run
    input: String,
}

fn main() -> anyhow::Result<()> {
    let BifferArgs {
        input: input_file_name,
    } = BifferArgs::parse();

    let mut input_file = OpenOptions::new()
        .read(true)
        .open(input_file_name)
        .context("Error openning the input file. Check file exists and permissions")?;

    let mut input = String::new();
    input_file.read_to_string(&mut input)?;

    let tokens = tokenize(&input);

    let mut interpreter =
        BasicInterpreter::new(tokens, "Hello world".as_bytes().to_owned(), Vec::new());
    interpreter.run();

    println!(
        "output size {}\n output:\n {}",
        interpreter.get_output().len(),
        String::from_utf8(interpreter.get_output().clone())?
    );

    Ok(())
}
