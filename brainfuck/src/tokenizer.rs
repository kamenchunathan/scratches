use std::iter::Enumerate;
use std::str::Chars;

use crate::{Token, TokenType};

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
        use TokenType::*;

        let mut jump_stack = Vec::new();

        let mut tokens: Vec<Token<()>> = Vec::with_capacity(self.input.len());
        while let Some((i, c)) = self.input_chars.next() {
            let tok_type = match c {
                '>' => Some(IncrDp),

                '<' => Some(DecrDp),

                '+' => Some(IncrByte),

                '-' => Some(DecrByte),

                '.' => Some(Output),

                ',' => Some(Input),

                '[' => {
                    jump_stack.push(tokens.len() as u32);
                    // u32::MAX is used as a sentinel value
                    Some(JmpLeft(u32::MAX))
                }

                ']' => {
                    let jmpl_token_idx = jump_stack
                        .pop()
                        .expect("Right bracket without matching LBracket");

                    // Assert that there is a matching uninitialized jumpleft
                    // before updating it's addres
                    debug_assert_eq!(tokens[jmpl_token_idx as usize].r#type, JmpLeft(u32::MAX));
                    tokens[jmpl_token_idx as usize].r#type = JmpLeft(tokens.len() as u32);

                    Some(JmpRight(jmpl_token_idx))
                }
                _ => {
                    // Any other character is treated as a comment
                    None
                }
            };

            if let Some(t_type) = tok_type {
                tokens.push(Token { r#type: t_type, data: () });
            }
        }

        assert!(jump_stack.is_empty());

        tokens
    }
}

#[cfg(test)]
mod test {
    use crate::{Token, TokenType, tokenizer::tokenize};

    #[test]
    fn single_incrdp() {
        use TokenType::*;
        let inp = ">";
        let tokens = tokenize(inp);

        assert_eq!(tokens[0].r#type, IncrDp);
    }

    #[test]
    fn single_decr_dp() {
        use TokenType::*;
        let inp = "<";
        let tokens = tokenize(inp);
        assert_eq!(tokens[0].r#type, DecrDp);
    }

    #[test]
    fn single_incr_byte() {
        use TokenType::*;
        let inp = "+";
        let tokens = tokenize(inp);
        assert_eq!(tokens[0].r#type, IncrByte);
    }

    #[test]
    fn single_decr_byte() {
        use TokenType::*;
        let inp = "-";
        let tokens = tokenize(inp);
        assert_eq!(tokens[0].r#type, DecrByte);
    }

    #[test]
    fn single_output() {
        use TokenType::*;
        let inp = ".";
        let tokens = tokenize(inp);
        assert_eq!(tokens[0].r#type, Output);
    }

    #[test]
    fn single_input() {
        use TokenType::*;
        let inp = ",";
        let tokens = tokenize(inp);
        assert_eq!(tokens[0].r#type, Input);
    }

    #[test]
    fn comments_are_ignored() {
        let inp = "><+-.,[]abcde";
        let tokens = tokenize(inp);
        use TokenType::*;
        assert_eq!(
            tokens.iter().map(|t| t.r#type).collect::<Vec<TokenType>>(),
            vec![
                IncrDp,
                DecrDp,
                IncrByte,
                DecrByte,
                Output,
                Input,
                JmpLeft(7),
                JmpRight(6)
            ]
        );
    }

    #[test]
    fn mixed_commands() {
        let inp = "++[><].,-";
        let tokens = tokenize(inp);
        use TokenType::*;
        assert_eq!(
            tokens.iter().map(|t| t.r#type).collect::<Vec<TokenType>>(),
            vec![
                IncrByte,
                IncrByte,
                JmpLeft(5),
                IncrDp,
                DecrDp,
                JmpRight(2),
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
        use TokenType::*;
        assert_eq!(tokens.iter().map(|t| t.r#type).collect::<Vec<TokenType>>(), vec![JmpLeft(1), JmpRight(0)]);
    }

    #[test]
    fn nested_loops() {
        let inp = "[[]]";
        let tokens = tokenize(inp);
        use TokenType::*;
        assert_eq!(
            tokens.iter().map(|t| t.r#type).collect::<Vec<TokenType>>(),
            vec![JmpLeft(3), JmpLeft(2), JmpRight(1), JmpRight(0)]
        );
    }

    #[test]
    fn complex_loop_structure() {
        let inp = "[[>+<]-]";
        let tokens = tokenize(inp);
        use TokenType::*;
        assert_eq!(
            tokens.iter().map(|t| t.r#type).collect::<Vec<TokenType>>(),
            vec![
                JmpLeft(7),
                JmpLeft(5),
                IncrDp,
                IncrByte,
                DecrDp,
                JmpRight(1),
                DecrByte,
                JmpRight(0)
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
