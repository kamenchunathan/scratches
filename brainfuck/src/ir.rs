// intermediate Representation where we can perform optimizations

use crate::instruction::Instruction;

/// an internal representation where optimizations are performed
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Expression {
    /// Move the data pointer by an offset
    OffsetDataPtr(i32),

    /// Add number to the value at the data pointer
    Add(i32),

    InputByte,

    OutputByte,

    JmpForwardIf0(u32),

    JmpBackIfNeq0(u32),
}

fn collapse_instrs(instr: &Instruction, num: i32) -> Expression {
    use Instruction::*;
    match instr {
        IncrDp => Expression::OffsetDataPtr(num),

        DecrDp => Expression::OffsetDataPtr(-num),

        IncrByte => Expression::Add(num),

        DecrByte => Expression::Add(-num),

        Output => Expression::OutputByte,

        Input => Expression::InputByte,

        JmpForwardIf0(ip) => Expression::JmpForwardIf0(*ip),

        JmpBackIfNeq0(ip) => Expression::JmpBackIfNeq0(*ip),
    }
}

pub fn optimize(instrs: Vec<Instruction>) -> Vec<Expression> {
    if instrs.is_empty() {
        return Vec::new();
    }

    let mut exprs = Vec::with_capacity(instrs.len() / 2);
    let mut earliest_equal_instr = 0;
    for i in 1..instrs.len() {
        if instrs[i] != instrs[earliest_equal_instr] {
            exprs.push(collapse_instrs(
                &instrs[earliest_equal_instr],
                i as i32 - earliest_equal_instr as i32,
            ));

            earliest_equal_instr = i;
        } else {
            use Instruction::*;
            match &instrs[i] {
                Output => exprs.push(Expression::OutputByte),

                Input => exprs.push(Expression::InputByte),

                JmpForwardIf0(ip) => exprs.push(Expression::JmpForwardIf0(*ip)),

                JmpBackIfNeq0(ip) => exprs.push(Expression::JmpBackIfNeq0(*ip)),

                _ => {}
            }
        }
    }

    // Handle last case
    exprs.push(collapse_instrs(
        &instrs[earliest_equal_instr],
        (instrs.len() - earliest_equal_instr) as i32,
    ));

    // Translate the addresses of the jump instructions to match the changed
    // instruction count
    let mut jmp_stack = Vec::new();
    for i in 0..exprs.len() {
        match &exprs[i] {
            Expression::JmpForwardIf0(_) => {
                jmp_stack.push(i);
            }

            Expression::JmpBackIfNeq0(_) => {
                let matching_idx = jmp_stack
                    .pop()
                    .expect("Every jump must have a matching address");
                exprs[matching_idx] = Expression::JmpForwardIf0(i as u32);
                exprs[i] = Expression::JmpBackIfNeq0(matching_idx as u32);
            }

            _ => {}
        }
    }

    exprs
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::instruction::Instruction::*;

    #[test]
    fn test_optimize_empty_input() {
        let instrs = vec![];
        let exprs = optimize(instrs);
        assert!(exprs.is_empty());
    }

    #[test]
    fn test_optimize_single_instructions() {
        let instrs = vec![IncrDp, DecrDp, IncrByte, DecrByte, Input, Output];
        let expected = vec![
            Expression::OffsetDataPtr(1),
            Expression::OffsetDataPtr(-1),
            Expression::Add(1),
            Expression::Add(-1),
            Expression::InputByte,
            Expression::OutputByte,
        ];
        let exprs = optimize(instrs);
        assert_eq!(exprs, expected);
    }

    #[test]
    fn test_optimize_adjacent_identical_instructions() {
        let instrs = vec![IncrDp, IncrDp, IncrDp, DecrByte, DecrByte];
        let expected = vec![Expression::OffsetDataPtr(3), Expression::Add(-2)];
        let exprs = optimize(instrs);
        assert_eq!(exprs, expected);
    }

    #[test]
    fn test_optimize_mixed_instruction_sequences() {
        let instrs = vec![IncrByte, IncrDp, IncrByte, DecrDp];
        let expected = vec![
            Expression::Add(1),
            Expression::OffsetDataPtr(1),
            Expression::Add(1),
            Expression::OffsetDataPtr(-1),
        ];
        let exprs = optimize(instrs);
        assert_eq!(exprs, expected);
    }

    #[test]
    fn test_optimize_jump_instructions() {
        let instrs = vec![
            JmpForwardIf0(5),
            IncrByte,
            IncrByte,
            IncrByte,
            IncrByte,
            JmpBackIfNeq0(0),
        ];
        let expected = vec![
            Expression::JmpForwardIf0(2),
            Expression::Add(4),
            Expression::JmpBackIfNeq0(0),
        ];
        let exprs = optimize(instrs);
        assert_eq!(exprs, expected);
    }

    #[test]
    fn test_optimize_complex_sequence() {
        let instrs = vec![
            IncrByte,
            IncrByte,
            IncrDp,
            IncrDp,
            DecrByte,
            DecrByte,
            DecrByte,
            Input,
            Output,
            JmpForwardIf0(10),
            IncrDp,
            JmpBackIfNeq0(8),
        ];
        let expected = vec![
            Expression::Add(2),
            Expression::OffsetDataPtr(2),
            Expression::Add(-3),
            Expression::InputByte,
            Expression::OutputByte,
            Expression::JmpForwardIf0(7), // Original JmpForwardIf0(10) now points to index 7
            Expression::OffsetDataPtr(1),
            Expression::JmpBackIfNeq0(5), // Original JmpBackIfNeq0(8) now points to index 5
        ];
        let exprs = optimize(instrs);
        assert_eq!(exprs, expected);
    }

    #[test]
    fn test_optimize_single_instruction_correctness() {
        let instrs = vec![IncrDp];
        let expected = vec![Expression::OffsetDataPtr(1)];
        let exprs = optimize(instrs);
        assert_eq!(exprs, expected);

        let instrs = vec![DecrDp];
        let expected = vec![Expression::OffsetDataPtr(-1)];
        let exprs = optimize(instrs);
        assert_eq!(exprs, expected);

        let instrs = vec![IncrByte];
        let expected = vec![Expression::Add(1)];
        let exprs = optimize(instrs);
        assert_eq!(exprs, expected);

        let instrs = vec![DecrByte];
        let expected = vec![Expression::Add(-1)];
        let exprs = optimize(instrs);
        assert_eq!(exprs, expected);

        let instrs = vec![Input];
        let expected = vec![Expression::InputByte];
        let exprs = optimize(instrs);
        assert_eq!(exprs, expected);

        let instrs = vec![Output];
        let expected = vec![Expression::OutputByte];
        let exprs = optimize(instrs);
        assert_eq!(exprs, expected);
    }
}
