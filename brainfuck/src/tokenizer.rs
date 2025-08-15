use std::iter::Enumerate;
use std::str::Chars;

use crate::TokenType;

fn tokenize(input: &str) -> Vec<TokenType> {
    let mut tokenizer = Tokenizer::new(input);
    tokenizer.tokenize()
}

struct Tokenizer<'a> {
    input: &'a str,
    input_chars: Enumerate<Chars<'a>>,
    idx: u32,
}

impl<'a> Tokenizer<'a> {
    fn new(input: &'a str) -> Self {
        Self {
            input,
            input_chars: input.chars().enumerate(),
            idx: 0,
        }
    }

    fn tokenize(&mut self) -> Vec<TokenType> {
        use TokenType::*;

        let mut jump_stack = Vec::new();

        let mut tokens = Vec::with_capacity(self.input.len());
        while let Some((i, c)) = self.input_chars.next() {
            let tok = match c {
                '>' => Some(IncrDp),

                '<' => Some(DecrDp),

                '+' => Some(IncrByte),

                '-' => Some(DecrByte),

                '.' => Some(Output),

                ',' => Some(Input),

                '[' => {
                    jump_stack.push(i as u32);
                    // u32::MAX is used as a sentinel value
                    Some(JmpLeft(u32::MAX))
                }

                ']' => {
                    let jmpl_addr = jump_stack
                        .pop()
                        .expect("Right bracket without matching LBracket");

                    // Assert that there is a matching uninitialized jumpleft
                    // before updating it's addres
                    debug_assert_eq!(tokens[jmpl_addr as usize], JmpLeft(u32::MAX));
                    tokens[jmpl_addr as usize] = JmpLeft(i as u32);

                    Some(JmpRight(jmpl_addr))
                }
                _ => {
                    // Any other character is treated as a comment
                    None
                }
            };

            if let Some(t) = tok {
                tokens.push(t);
            }
        }

        tokens
    }
}

// #[cfg(test)]
// mod test {
//     use crate::{TokenType, tokenizer::tokenize};
//
//     #[test]
//
//     fn single_incrdp() {
//         use TokenType::*;
//         let inp = ">";
//         let tokens = tokenize(inp);
//
//         assert_eq!(tokens, vec![IncrDp]);
//     }
//
//     #[test]
//     fn single_decr_dp() {
//         use TokenType::*;
//         let inp = "<";
//         let tokens = tokenize(inp);
//         assert_eq!(tokens, vec![DecrDp]);
//     }
//
//     #[test]
//     fn single_incr_byte() {
//         use TokenType::*;
//         let inp = "+";
//         let tokens = tokenize(inp);
//         assert_eq!(tokens, vec![IncrByte]);
//     }
//
//     #[test]
//     fn single_decr_byte() {
//         use TokenType::*;
//         let inp = "-";
//         let tokens = tokenize(inp);
//         assert_eq!(tokens, vec![DecrByte]);
//     }
//
//     #[test]
//     fn single_output() {
//         use TokenType::*;
//         let inp = ".";
//         let tokens = tokenize(inp);
//         assert_eq!(tokens, vec![Output]);
//     }
//
//     #[test]
//     fn single_input() {
//         use TokenType::*;
//         let inp = ",";
//         let tokens = tokenize(inp);
//         assert_eq!(tokens, vec![Input]);
//     }
//
//     #[test]
//     fn single_jmp_left() {
//         use TokenType::*;
//         let inp = "[";
//         let tokens = tokenize(inp);
//         assert_eq!(tokens, vec![JmpLeft]);
//     }
//
//     #[test]
//     fn single_jmp_right() {
//         use TokenType::*;
//         let inp = "]";
//         let tokens = tokenize(inp);
//         assert_eq!(tokens, vec![JmpRight]);
//     }
//
//     #[test]
//     fn comments_are_ignored() {
//         let inp = "><+-.,[]abcde";
//         let tokens = tokenize(inp);
//         use TokenType::*;
//         assert_eq!(
//             tokens,
//             vec![
//                 IncrDp, DecrDp, IncrByte, DecrByte, Output, Input, JmpLeft, JmpRight
//             ]
//         );
//     }
//
//     #[test]
//     fn mixed_commands() {
//         let inp = "++[><].,-";
//         let tokens = tokenize(inp);
//         use TokenType::*;
//         assert_eq!(
//             tokens,
//             vec![
//                 IncrByte, IncrByte, JmpLeft, IncrDp, DecrDp, JmpRight, Output, Input, DecrByte
//             ]
//         );
//     }
//
//     #[test]
//     fn empty_input() {
//         let inp = "";
//         let tokens = tokenize(inp);
//         assert!(tokens.is_empty());
//     }
//
//     #[test]
//     fn only_comments() {
//         let inp = "hello world";
//         let tokens = tokenize(inp);
//         assert!(tokens.is_empty());
//     }
// }
