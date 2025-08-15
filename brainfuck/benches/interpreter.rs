use criterion::{Criterion, criterion_group, criterion_main};

use biffer::{interpreter::basic::BasicInterpreter, token::tokenize};

fn bench_play_game(c: &mut Criterion) {
    let input = include_str!("../bf/morse_code.bf");

    let tokens = tokenize(input);

    const MESSAGE: &str = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

    let mut interpreter = BasicInterpreter::new(tokens, MESSAGE.as_bytes().to_owned(), Vec::new());

    c.bench_function("bench interpreter ", |b| {
        b.iter(|| {
            std::hint::black_box(for _ in 1..=100 {
                interpreter.run();
            });
        });
    });
}

criterion_group!(benches, bench_play_game,);
criterion_main!(benches);
