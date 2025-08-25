use serde::{Deserialize, Serialize};
use std::collections::HashMap;

#[derive(Serialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct GeminiRequest {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub system_instruction: Option<SystemInstruction>,
    pub contents: Vec<Content>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub tools: Option<Vec<Tool>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub tool_config: Option<ToolConfig>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub generation_config: Option<GenerationConfig>,
}

#[derive(Serialize, Debug, Clone)]
pub struct SystemInstruction {
    pub parts: Vec<TextPart>,
}

#[derive( Deserialize, Serialize, Debug, Clone)]
pub struct TextPart {
    pub text: String,
}

#[derive(Serialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct GenerationConfig {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub temperature: Option<f32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub max_output_tokens: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub top_p: Option<f32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub top_k: Option<u32>,
}

#[derive(Serialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Tool {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub function_declarations: Option<Vec<FunctionDeclaration>>,
}

#[derive(Serialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct ToolConfig {
    pub function_calling_config: FunctionCallingConfig,
}

#[derive(Serialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct FunctionCallingConfig {
    pub mode: FunctionCallingMode,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub allowed_function_names: Option<Vec<String>>,
}

#[derive(Serialize, Debug, Clone)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
pub enum FunctionCallingMode {
    Auto,
    Any,
    None,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct FunctionDeclaration {
    pub name: String,
    pub description: String,
    pub parameters: JsonSchema,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct JsonSchema {
    #[serde(rename = "type")]
    pub type_name: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub properties: Option<HashMap<String, JsonSchemaProperty>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub required: Option<Vec<String>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub items: Option<Box<JsonSchemaProperty>>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct JsonSchemaProperty {
    #[serde(rename = "type")]
    pub type_name: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub description: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub items: Option<Box<JsonSchemaProperty>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub r#enum: Option<Vec<String>>,
}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct GeminiAPIResponse {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub model_version: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub response_id: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub usage_metadata: Option<UsageMetadata>,
    pub candidates: Vec<Candidate>,
}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Candidate {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub avg_logprobs: Option<f64>,
    pub content: Content,
    pub finish_reason: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub citation_metadata: Option<CitationMetadata>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub token_count: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub grounding_attributions: Option<Vec<GroundingAttribution>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub safety_ratings: Option<Vec<SafetyRating>>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct Content {
    pub parts: Vec<Part>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub role: Option<String>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(untagged)]
pub enum Part {
    Text(TextPart),
    FunctionCall(FunctionCallPart),
    FunctionResponse(FunctionResponsePart),
}

impl Part {
    pub fn text(content: impl Into<String>) -> Self {
        Part::Text(TextPart { text: content.into() })
    }

    pub fn function_call(name: impl Into<String>, args: serde_json::Value) -> Self {
        Part::FunctionCall(FunctionCallPart {
            function_call: FunctionCall {
                name: name.into(),
                args,
            },
        })
    }

    pub fn function_response(name: impl Into<String>, response: serde_json::Value) -> Self {
        Part::FunctionResponse(FunctionResponsePart {
            function_response: FunctionResponse {
                name: name.into(),
                response,
            },
        })
    }
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct FunctionCallPart {
    pub function_call: FunctionCall,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct FunctionCall {
    pub name: String,
    pub args: serde_json::Value,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct FunctionResponsePart {
    pub function_response: FunctionResponse,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct FunctionResponse {
    pub name: String,
    pub response: serde_json::Value,
}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct UsageMetadata {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub candidates_token_count: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub candidates_tokens_details: Option<Vec<TokenDetails>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub prompt_token_count: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub prompt_tokens_details: Option<Vec<TokenDetails>>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub total_token_count: Option<u32>,
}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct TokenDetails {
    pub modality: String,
    pub token_count: u32,
}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct CitationMetadata {
    pub citation_sources: Vec<CitationSource>,
}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct CitationSource {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub start_index: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub end_index: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub uri: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub license: Option<String>,
}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct GroundingAttribution {
    #[serde(skip_serializing_if = "Option::is_none")]
    pub source_id: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub confidence_score: Option<f32>,
}

#[derive(Deserialize, Debug, Clone)]
#[serde(rename_all = "camelCase")]
pub struct SafetyRating {
    pub category: String,
    pub probability: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub blocked: Option<bool>,
}

impl GeminiRequest {
    pub fn new(contents: Vec<Content>) -> Self {
        Self {
            system_instruction: None,
            contents,
            tools: None,
            tool_config: None,
            generation_config: None,
        }
    }

    pub fn with_system_instruction(mut self, instruction: impl Into<String>) -> Self {
        self.system_instruction = Some(SystemInstruction {
            parts: vec![TextPart { text: instruction.into() }],
        });
        self
    }

    pub fn with_tools(mut self, tools: Vec<Tool>) -> Self {
        self.tools = Some(tools);
        self
    }

    pub fn with_function_declarations(mut self, functions: Vec<FunctionDeclaration>) -> Self {
        let tool = Tool {
            function_declarations: Some(functions),
        };
        self.tools = Some(vec![tool]);
        self
    }

    pub fn with_tool_config(mut self, mode: FunctionCallingMode, allowed_functions: Option<Vec<String>>) -> Self {
        self.tool_config = Some(ToolConfig {
            function_calling_config: FunctionCallingConfig {
                mode,
                allowed_function_names: allowed_functions,
            },
        });
        self
    }

    pub fn with_generation_config(mut self, config: GenerationConfig) -> Self {
        self.generation_config = Some(config);
        self
    }
}

impl Content {
    pub fn user(text: impl Into<String>) -> Self {
        Self {
            parts: vec![Part::text(text)],
            role: Some("user".to_string()),
        }
    }

    pub fn model(parts: Vec<Part>) -> Self {
        Self {
            parts,
            role: Some("model".to_string()),
        }
    }

    pub fn function_response_content(name: impl Into<String>, response: serde_json::Value) -> Self {
        Self {
            parts: vec![Part::function_response(name, response)],
            role: Some("user".to_string()),
        }
    }
}

impl GenerationConfig {
    pub fn new() -> Self {
        Self {
            temperature: None,
            max_output_tokens: None,
            top_p: None,
            top_k: None,
        }
    }

    pub fn with_temperature(mut self, temperature: f32) -> Self {
        self.temperature = Some(temperature);
        self
    }

    pub fn with_max_tokens(mut self, max_tokens: u32) -> Self {
        self.max_output_tokens = Some(max_tokens);
        self
    }
}

impl GeminiAPIResponse {
    pub fn has_function_calls(&self) -> bool {
        self.candidates.iter().any(|c| c.has_function_calls())
    }

    pub fn get_function_calls(&self) -> Vec<&FunctionCall> {
        self.candidates
            .first()
            .map(|c| c.get_function_calls())
            .unwrap_or_default()
    }

    pub fn get_text(&self) -> Option<String> {
        self.candidates
            .first()
            .and_then(|c| c.get_text())
    }
}

impl Candidate {
    pub fn has_function_calls(&self) -> bool {
        self.content.parts.iter().any(|p| matches!(p, Part::FunctionCall(_)))
    }

    pub fn get_function_calls(&self) -> Vec<&FunctionCall> {
        self.content.parts
            .iter()
            .filter_map(|p| match p {
                Part::FunctionCall(fc) => Some(&fc.function_call),
                _ => None,
            })
            .collect()
    }

    pub fn get_text(&self) -> Option<String> {
        let text_parts: Vec<String> = self.content.parts
            .iter()
            .filter_map(|p| match p {
                Part::Text(t) => Some(t.text.clone()),
                _ => None,
            })
            .collect();
        
        if text_parts.is_empty() {
            None
        } else {
            Some(text_parts.join(""))
        }
    }
}


