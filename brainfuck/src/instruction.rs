use std::iter::Enumerate;
use std::str::Chars;

#[derive(Debug, Clone)]
pub struct Token<T = ()> {
    pub r#type: Instruction,
    pub data: T,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Instruction {
    /// > Increment data pointer
    IncrDp,

    /// < Decrement the data pointer
    DecrDp,

    /// + Increment the byte at the data pointer by one.
    IncrByte,

    ///  - Decrement the byte at the data pointer by one.
    DecrByte,

    /// . Output the byte at the data pointer.
    Output,

    /// , Accept one byte of input, storing its value in the byte at the data
    /// pointer.
    Input,

    /// [ If the byte at the data pointer is zero, then instead of moving the
    /// instruction pointer forward to the next command, jump it forward to the
    /// command after the matching ] command.
    JmpForwardIf0(u32),

    /// ] If the byte at the data pointer is nonzero, then instead of moving the
    /// instruction pointer forward to the next command, jump it back to the
    /// command after the matching [ command
    JmpBackIfNeq0(u32),
}

pub fn tokenize(input: &str) -> Vec<Token<()>> {
    let mut tokenizer = Tokenizer::new(input);
    tokenizer.tokenize()
}

struct Tokenizer<'a> {
    input: &'a str,
    input_chars: Enumerate<Chars<'a>>,
}

impl<'a> Tokenizer<'a> {
    fn new(input: &'a str) -> Self {
        Self {
            input,
            input_chars: input.chars().enumerate(),
        }
    }

    fn tokenize(&mut self) -> Vec<Token<()>> {
        use Instruction::*;

        let mut jump_stack = Vec::new();
        let mut tokens: Vec<Token<()>> = Vec::with_capacity(self.input.len());
        while let Some((_i, c)) = self.input_chars.next() {
            let tok_type = match c {
                '>' => Some(IncrDp),

                '<' => Some(DecrDp),

                '+' => Some(IncrByte),

                '-' => Some(DecrByte),

                '.' => Some(Output),

                ',' => Some(Input),

                '[' => {
                    jump_stack.push(tokens.len());
                    // u32::MAX is used as a sentinel value
                    Some(JmpForwardIf0(u32::MAX))
                }

                ']' => {
                    let jmpl_token_idx = jump_stack
                        .pop()
                        .expect("Right bracket without matching LBracket");

                    // Assert that there is a matching uninitialized jumpleft
                    // before updating it's addres
                    debug_assert_eq!(
                        tokens[jmpl_token_idx as usize].r#type,
                        JmpForwardIf0(u32::MAX)
                    );
                    tokens[jmpl_token_idx as usize].r#type = JmpForwardIf0(tokens.len() as u32);

                    Some(JmpBackIfNeq0(jmpl_token_idx as u32))
                }
                _ => {
                    // Any other character is treated as a comment
                    None
                }
            };

            if let Some(t_type) = tok_type {
                tokens.push(Token {
                    r#type: t_type,
                    data: (),
                });
            }
        }

        assert!(jump_stack.is_empty());

        tokens
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn single_incrdp() {
        use Instruction::*;
        let inp = ">";
        let tokens = tokenize(inp);

        assert_eq!(tokens[0].r#type, IncrDp);
    }

    #[test]
    fn single_decr_dp() {
        use Instruction::*;
        let inp = "<";
        let tokens = tokenize(inp);
        assert_eq!(tokens[0].r#type, DecrDp);
    }

    #[test]
    fn single_incr_byte() {
        use Instruction::*;
        let inp = "+";
        let tokens = tokenize(inp);
        assert_eq!(tokens[0].r#type, IncrByte);
    }

    #[test]
    fn single_decr_byte() {
        use Instruction::*;
        let inp = "-";
        let tokens = tokenize(inp);
        assert_eq!(tokens[0].r#type, DecrByte);
    }

    #[test]
    fn single_output() {
        use Instruction::*;
        let inp = ".";
        let tokens = tokenize(inp);
        assert_eq!(tokens[0].r#type, Output);
    }

    #[test]
    fn single_input() {
        use Instruction::*;
        let inp = ",";
        let tokens = tokenize(inp);
        assert_eq!(tokens[0].r#type, Input);
    }

    #[test]
    fn comments_are_ignored() {
        let inp = "><+-.,[]abcde";
        let tokens = tokenize(inp);
        use Instruction::*;
        assert_eq!(
            tokens
                .iter()
                .map(|t| t.r#type)
                .collect::<Vec<Instruction>>(),
            vec![
                IncrDp,
                DecrDp,
                IncrByte,
                DecrByte,
                Output,
                Input,
                JmpForwardIf0(7),
                JmpBackIfNeq0(6)
            ]
        );
    }

    #[test]
    fn mixed_commands() {
        let inp = "++[><].,-";
        let tokens = tokenize(inp);
        use Instruction::*;
        assert_eq!(
            tokens
                .iter()
                .map(|t| t.r#type)
                .collect::<Vec<Instruction>>(),
            vec![
                IncrByte,
                IncrByte,
                JmpForwardIf0(5),
                IncrDp,
                DecrDp,
                JmpBackIfNeq0(2),
                Output,
                Input,
                DecrByte
            ]
        );
    }

    #[test]
    fn empty_input() {
        let inp = "";
        let tokens = tokenize(inp);
        assert!(tokens.is_empty());
    }

    #[test]
    fn only_comments() {
        let inp = "hello world";
        let tokens = tokenize(inp);
        assert!(tokens.is_empty());
    }

    #[test]
    fn simple_loop() {
        let inp = "[]";
        let tokens = tokenize(inp);
        use Instruction::*;
        assert_eq!(
            tokens
                .iter()
                .map(|t| t.r#type)
                .collect::<Vec<Instruction>>(),
            vec![JmpForwardIf0(1), JmpBackIfNeq0(0)]
        );
    }

    #[test]
    fn nested_loops() {
        let inp = "[[]]";
        let tokens = tokenize(inp);
        use Instruction::*;
        assert_eq!(
            tokens
                .iter()
                .map(|t| t.r#type)
                .collect::<Vec<Instruction>>(),
            vec![
                JmpForwardIf0(3),
                JmpForwardIf0(2),
                JmpBackIfNeq0(1),
                JmpBackIfNeq0(0)
            ]
        );
    }

    #[test]
    fn complex_loop_structure() {
        let inp = "[[>+<]-]";
        let tokens = tokenize(inp);
        use Instruction::*;
        assert_eq!(
            tokens
                .iter()
                .map(|t| t.r#type)
                .collect::<Vec<Instruction>>(),
            vec![
                JmpForwardIf0(7),
                JmpForwardIf0(5),
                IncrDp,
                IncrByte,
                DecrDp,
                JmpBackIfNeq0(1),
                DecrByte,
                JmpBackIfNeq0(0)
            ]
        );
    }

    #[test]
    #[should_panic(expected = "Right bracket without matching LBracket")]
    fn unmatched_right_bracket() {
        let inp = "]";
        tokenize(inp);
    }

    #[test]
    #[should_panic]
    fn unmatched_left_bracket() {
        let inp = "[";
        tokenize(inp);
    }
}
