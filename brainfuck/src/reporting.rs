pub struct Loc {
    line: u32,
    column: u32,
}

pub struct Span {
    start: Loc,
    end: Loc,
}
