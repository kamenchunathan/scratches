// A basic interpreter that works on tokens directly without Optimizing any
// instructions

use crate::token::{Token, TokenType};

#[derive(Debug)]
pub struct BasicInterpreter {
    instr_ptr: u32,
    data_ptr: u32,
    memory: Vec<u8>,
    input: Vec<u8>,
    output: Vec<u8>,
    tokens: Vec<Token>,
}

impl BasicInterpreter {
    const MAX_MEMORY_SIZE: u32 = 1024 * 1024;
    const MAX_INSTR_COUNT: u32 = 1024 * 1024 * 1024;

    pub fn new(tokens: Vec<Token>, input: Vec<u8>, output: Vec<u8>) -> Self {
        Self {
            memory: vec![0],
            input,
            output,
            tokens,
            instr_ptr: 0,
            data_ptr: 0,
        }
    }

    pub fn run(&mut self) {
        use TokenType::*;
        let mut instr_count = 0;
        loop {
            instr_count += 1;
            if instr_count >= Self::MAX_INSTR_COUNT {
                eprintln!("max instruction count reached");
                break;
            }

            let tok = &self.tokens[self.instr_ptr as usize];

            // println!(
            //     "{:?} inp {:?} out {:?} mem {:?} cur {:?}",
            //     &tok.r#type, &self.input, self.output, self.memory, self.data_ptr
            // );
            match tok.r#type {
                IncrDp => {
                    self.data_ptr += 1;
                }

                DecrDp => {
                    self.data_ptr -= 1;
                }

                IncrByte => {
                    let success = self.set_mem_value(
                        self.data_ptr,
                        self.get_mem_value(self.data_ptr).overflowing_add(1).0,
                    );

                    if !success {
                        break;
                    }
                }

                DecrByte => {
                    let success = self.set_mem_value(
                        self.data_ptr,
                        self.get_mem_value(self.data_ptr).overflowing_sub(1).0,
                    );

                    if !success {
                        break;
                    }
                }

                Output => {
                    self.output.push(self.get_mem_value(self.data_ptr));
                }

                Input => {
                    if self.input.is_empty() {
                        break;
                    }

                    let inp = self.input.remove(0);
                    self.set_mem_value(self.data_ptr, inp);
                }

                JmpLeft(instr) => {
                    if self.get_mem_value(self.data_ptr) == 0 {
                        self.instr_ptr = instr + 1;
                        continue;
                    }
                }

                JmpRight(instr) => {
                    if self.get_mem_value(self.data_ptr) != 0 {
                        self.instr_ptr = instr + 1;
                        continue;
                    }
                }
            }

            self.instr_ptr += 1;
            if self.instr_ptr >= self.tokens.len() as u32 {
                break;
            }
        }
    }

    pub fn get_output(&self) -> &Vec<u8> {
        &self.output
    }

    pub fn get_mem_value(&self, index: u32) -> u8 {
        if index < self.memory.len() as u32 {
            self.memory[index as usize]
        } else {
            0
        }
    }

    fn set_mem_value(&mut self, index: u32, value: u8) -> bool {
        if index >= self.memory.len() as u32 {
            if self.memory.len() >= Self::MAX_MEMORY_SIZE as usize {
                eprintln!("Attempted allocation beyond memory size");
                return false;
            }
            // "Allocate" memory
            self.memory.extend(std::iter::repeat_n(
                0,
                index as usize - self.memory.len() + 1,
            ));
        }

        self.memory[index as usize] = value;
        return true;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn create_interpreter(code: &str, input: &str) -> BasicInterpreter {
        let tokens = crate::token::tokenize(code);
        BasicInterpreter::new(tokens, input.as_bytes().to_vec(), Vec::new())
    }

    #[test]
    fn test_incr_dp() {
        let mut interpreter = create_interpreter(">", "");
        interpreter.run();
        assert_eq!(interpreter.data_ptr, 1);
    }

    #[test]
    fn test_decr_dp() {
        let mut interpreter = create_interpreter(">>", ""); // Move right twice
        interpreter.run();
        assert_eq!(interpreter.data_ptr, 2);

        let mut interpreter = create_interpreter("><", ""); // Move right then left
        interpreter.run();
        assert_eq!(interpreter.data_ptr, 0);
    }

    #[test]
    fn test_incr_byte() {
        let mut interpreter = create_interpreter("+", "");
        interpreter.run();
        assert_eq!(interpreter.get_mem_value(0), 1);

        let mut interpreter = create_interpreter("++", "");
        interpreter.run();
        assert_eq!(interpreter.get_mem_value(0), 2);
    }

    #[test]
    fn test_decr_byte() {
        let mut interpreter = create_interpreter("+-", "");
        interpreter.run();
        assert_eq!(interpreter.get_mem_value(0), 0);

        let mut interpreter = create_interpreter("+++-", "");
        interpreter.run();
        assert_eq!(interpreter.get_mem_value(0), 2);
    }

    #[test]
    fn test_output() {
        let mut interpreter = create_interpreter("+.+.", "");
        interpreter.run();
        assert_eq!(interpreter.get_output(), &vec![1, 2]);
    }

    #[test]
    fn test_input() {
        let mut interpreter = create_interpreter(",", "A");
        interpreter.run();
        assert_eq!(interpreter.get_mem_value(0), b'A');

        let mut interpreter = create_interpreter(",>,", "BC");
        interpreter.run();
        assert_eq!(interpreter.get_mem_value(0), b'B');
        assert_eq!(interpreter.get_mem_value(1), b'C');
    }

    #[test]
    fn test_simple_loop() {
        // Set cell 0 to 5, then loop 5 times decrementing it to 0
        let mut interpreter = create_interpreter("+++++[->-]<", "");
        interpreter.run();
        assert_eq!(interpreter.get_mem_value(0), 0);
    }

    #[test]
    fn test_hello_world() {
        let code = "+[----->+++<]>+.---.+++++++..+++.[--->+<]>-----.--[->++++<]>-.--------.+++.------.--------.";

        let mut interpreter = create_interpreter(code, "");
        interpreter.run();
        let expected_output = "hello world".as_bytes().to_vec();
        assert_eq!(interpreter.get_output(), &expected_output);
    }

    #[test]
    fn test_cat_program() {
        let code = ",[.,]"; // Read a character, print it, loop until 0
        let input = "Rust\n";
        let mut interpreter = create_interpreter(code, input);
        interpreter.run();
        assert_eq!(interpreter.get_output(), input.as_bytes());
    }
}
