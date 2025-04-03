pub mod db;

use std::{cmp::Ordering, collections::HashMap, path::Path, sync::OnceLock};

use pgn_reader::{self, RawHeader, SanPlus, Visitor};
use shakmaty::{Board, Chess, Color, Outcome, Position};
use shakmaty_syzygy::{self, AmbiguousWdl, Tablebase};

use db::GameSummary;

#[derive(Debug)]
pub struct SummaryAnalysis {
    position: Chess,
    tables: Tablebase<Chess>,
    white_elo: Option<u32>,
    black_elo: Option<u32>,
    termination: String,
    actual_outcome: Option<shakmaty::Outcome>,
    endgame_sequence: String,
    t_5men: String,
    t_4men: String,
    t_3men: String,
    t_5men_wdl: Option<String>,
    t_4men_wdl: Option<String>,
    t_3men_wdl: Option<String>,
    move_no: u32,
    cur_player: Color,
}

impl SummaryAnalysis {
    pub fn new() -> Self {
        let mut tables = Tablebase::new();
        tables.add_directory(Path::new("./data/tablebase")).unwrap();

        Self {
            tables,
            move_no: 1,
            // Assuming that all games start from the beginning
            cur_player: Color::White,
            position: Chess::new(),
            white_elo: None,
            black_elo: None,
            termination: String::new(),
            actual_outcome: None,
            endgame_sequence: String::new(),
            t_5men: String::new(),
            t_4men: String::new(),
            t_3men: String::new(),
            t_5men_wdl: None,
            t_4men_wdl: None,
            t_3men_wdl: None,
        }
    }
}

impl Visitor for SummaryAnalysis {
    type Result = GameSummary;

    fn header(&mut self, key: &[u8], value: RawHeader<'_>) {
        let key = String::from_utf8(key.to_owned()).unwrap();
        let value = String::from_utf8(value.decode().to_vec()).unwrap();
        match key.as_str() {
            "WhiteElo" => self.white_elo = Some(value.parse().unwrap()),
            "BlackElo" => self.black_elo = Some(value.parse().unwrap()),
            "Termination" => {
                self.termination = value.split(" ").last().unwrap_or_default().to_string()
            }
            _ => {}
        }
    }

    fn san(&mut self, notation: SanPlus) {
        let Ok(mov) = notation.san.to_move(&self.position) else {
            return;
        };

        self.position = self.position.clone().play(&mov).unwrap();
        let board = self.position.board();
        let board_iter = board.iter();
        match board_iter.len() {
            5 if self.t_5men.is_empty() => {
                self.t_5men = endgame_tag(board);
                self.t_5men_wdl = self
                    .tables
                    .probe_wdl(&self.position)
                    .ok()
                    .map(|wdl| repr_ambiguous_wdl(wdl).to_string())
            }
            4 if self.t_4men.is_empty() => {
                self.t_4men = endgame_tag(board);
                self.t_4men_wdl = self
                    .tables
                    .probe_wdl(&self.position)
                    .ok()
                    .map(|wdl| repr_ambiguous_wdl(wdl).to_string())
            }
            3 if self.t_3men.is_empty() => {
                self.t_3men = endgame_tag(board);
                self.t_3men_wdl = self
                    .tables
                    .probe_wdl(&self.position)
                    .ok()
                    .map(|wdl| repr_ambiguous_wdl(wdl).to_string())
            }
            _ => {}
        }

        if board_iter.len() < 6 {
            match self.cur_player {
                Color::White => {
                    self.endgame_sequence
                        .push_str(format!("{}. {} ", self.move_no, notation.to_string()).as_str());
                }
                Color::Black => {
                    self.endgame_sequence
                        .push_str(notation.to_string().as_str());
                    self.endgame_sequence.push(' ');
                }
            }
        }

        self.cur_player = match self.cur_player {
            Color::Black => Color::White,
            Color::White => Color::Black,
        };
        self.move_no += 1;
    }

    fn outcome(&mut self, outcome: Option<shakmaty::Outcome>) {
        self.actual_outcome = outcome;
    }

    fn end_game(&mut self) -> Self::Result {
        let actual_outcome = match self.actual_outcome.unwrap() {
            Outcome::Decisive {
                winner: Color::White,
            } => "w",
            Outcome::Decisive {
                winner: Color::Black,
            } => "b",
            Outcome::Draw => "d",
        }
        .to_string();

        GameSummary {
            white_elo: self.white_elo,
            black_elo: self.black_elo,
            t_5men: self.t_5men.clone(),
            t_4men: self.t_4men.clone(),
            t_3men: self.t_3men.clone(),
            t_5men_wdl: self.t_5men_wdl.clone(),
            t_4men_wdl: self.t_4men_wdl.clone(),
            t_3men_wdl: self.t_3men_wdl.clone(),
            actual_outcome,
            termination: self.termination.clone(),
            endgame_sequence: self.endgame_sequence.clone(),
            end_piece_count: self.position.board().iter().len() as u32,
        }
    }
}

fn piece_values() -> &'static HashMap<char, u32> {
    static VALS: OnceLock<HashMap<char, u32>> = OnceLock::new();
    VALS.get_or_init(|| {
        let mut piece_values = HashMap::new();
        piece_values.insert('p', 1);
        piece_values.insert('n', 2);
        piece_values.insert('b', 3);
        piece_values.insert('r', 5);
        piece_values.insert('q', 9);
        piece_values.insert('k', 10);
        piece_values
    })
}

fn endgame_tag(board: &Board) -> String {
    let vals = piece_values();
    let mut pieces = board
        .iter()
        .map(|(_, piece)| piece.char())
        .collect::<Vec<char>>();
    pieces.sort_by(|a, b| match (a.is_uppercase(), b.is_uppercase()) {
        (true, false) => Ordering::Greater,
        (false, true) => Ordering::Less,
        (_, _) => Ord::cmp(
            vals.get(&a.to_lowercase().next().unwrap()).unwrap(),
            vals.get(&b.to_lowercase().next().unwrap()).unwrap(),
        ),
    });
    pieces.reverse();
    let mut black_idx = 0;
    for i in 0..pieces.len() {
        if pieces[i].is_lowercase() {
            black_idx = i;
            break;
        }
    }

    let (w, b) = pieces.split_at(black_idx);
    format!(
        "{}v{}",
        w.iter().collect::<String>(),
        b.iter().collect::<String>().to_uppercase(),
    )
}

fn repr_ambiguous_wdl(wdl: AmbiguousWdl) -> &'static str {
    use AmbiguousWdl::*;
    match wdl {
        Loss => "l",
        MaybeLoss => "ml",
        BlessedLoss => "bl",
        Draw => "d",
        CursedWin => "cw",
        MaybeWin => "mw",
        Win => "w",
    }
}
