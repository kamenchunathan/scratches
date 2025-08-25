use crate::types::{FunctionDeclaration, JsonSchema, JsonSchemaProperty};
use std::collections::HashMap;

pub fn query_board_state() -> FunctionDeclaration {
    FunctionDeclaration {
        name: "query_board_state".to_string(),
        description: "Returns the current board state in FEN notation".to_string(),
        parameters: JsonSchema {
            type_name: "object".to_string(),
            properties: Some(HashMap::new()),
            required: Some(vec![]),
            items: None,
        },
    }
}

pub fn query_move_history() -> FunctionDeclaration {
    let mut properties = HashMap::new();
    
    properties.insert(
        "path_id".to_string(),
        JsonSchemaProperty {
            type_name: "string".to_string(),
            description: Some("The path ID to get move history for".to_string()),
            items: None,
            r#enum: None,
        },
    );

    FunctionDeclaration {
        name: "query_move_history".to_string(),
        description: "Takes in a path id and returns the move history for that path id".to_string(),
        parameters: JsonSchema {
            type_name: "object".to_string(),
            properties: Some(properties),
            required: Some(vec!["path_id".to_string()]),
            items: None,
        },
    }
}

pub fn create_fork() -> FunctionDeclaration {
    let mut properties = HashMap::new();
    
    properties.insert(
        "moves".to_string(),
        JsonSchemaProperty {
            type_name: "array".to_string(),
            description: Some("Sequence of moves to evaluate (max 4 moves deep)".to_string()),
            items: Some(Box::new(JsonSchemaProperty {
                type_name: "string".to_string(),
                description: Some("Move in algebraic notation".to_string()),
                items: None,
                r#enum: None,
            })),
            r#enum: None,
        },
    );

    FunctionDeclaration {
        name: "create_fork".to_string(),
        description: "Create a fork to evaluate different move sequences. Maximum 3 forks at any point with depth of 4 moves. Forks are deleted when a move is made. Returns a path_id for the fork.".to_string(),
        parameters: JsonSchema {
            type_name: "object".to_string(),
            properties: Some(properties),
            required: Some(vec!["moves".to_string()]),
            items: None,
        },
    }
}

pub fn query_valid_moves() -> FunctionDeclaration {
    let mut properties = HashMap::new();
    
    properties.insert(
        "piece_position".to_string(),
        JsonSchemaProperty {
            type_name: "string".to_string(),
            description: Some("The position of the piece to query valid moves for (e.g., 'e4', 'a1')".to_string()),
            items: None,
            r#enum: None,
        },
    );

    FunctionDeclaration {
        name: "query_valid_moves".to_string(),
        description: "Takes a piece position and returns valid moves from that position".to_string(),
        parameters: JsonSchema {
            type_name: "object".to_string(),
            properties: Some(properties),
            required: Some(vec!["piece_position".to_string()]),
            items: None,
        },
    }
}

pub fn query_attacking_pieces() -> FunctionDeclaration {
    let mut properties = HashMap::new();
    
    properties.insert(
        "square".to_string(),
        JsonSchemaProperty {
            type_name: "string".to_string(),
            description: Some("The square to query for attacking pieces (e.g., 'e4', 'h8')".to_string()),
            items: None,
            r#enum: None,
        },
    );

    FunctionDeclaration {
        name: "query_attacking_pieces".to_string(),
        description: "Takes a square and returns a set of enemy pieces in order of increasing value that are attacking that square".to_string(),
        parameters: JsonSchema {
            type_name: "object".to_string(),
            properties: Some(properties),
            required: Some(vec!["square".to_string()]),
            items: None,
        },
    }
}

pub fn all_chess_functions() -> Vec<FunctionDeclaration> {
    vec![
        query_board_state(),
        query_move_history(),
        create_fork(),
        query_valid_moves(),
        query_attacking_pieces(),
    ]
}

pub fn chess_system_prompt() -> String {
    r#"You are a chess master AI. Your role is to analyze positions and make the best moves possible.

For each position, you must:
1. Consider exactly three candidate moves - these MUST be valid moves
2. For each candidate move, create a fork using create_fork() to evaluate the resulting position up to 4 moves deep
3. Analyze each fork's end position for tactical and strategic value
4. Choose and make the best move based on your analysis

Your evaluation process:
- Use query_board_state() to understand the current position
- Use query_valid_moves() to ensure your candidate moves are legal
- Use query_attacking_pieces() to assess threats and tactics
- Use create_fork() to explore each candidate move sequence
- Consider material, king safety, piece activity, and positional factors

Always explain your reasoning for each candidate move and why you selected the final move."#.to_string()
}
