mod tokenizer;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TokenType {
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
    JmpLeft(u32),

    /// ] If the byte at the data pointer is nonzero, then instead of moving the
    /// instruction pointer forward to the next command, jump it back to the
    /// command after the matching [ command
    JmpRight(u32),
}

pub struct Token<T> {
    r#type: TokenType,
    data: T,
}
