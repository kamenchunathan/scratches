use crate::types::{GeminiRequest, GeminiAPIResponse, Content, FunctionDeclaration, GenerationConfig, FunctionCallingMode};
use reqwest;
use std::error::Error;

pub struct GeminiClient {
    client: reqwest::Client,
    api_key: String,
    base_url: String,
    system_prompt: Option<String>,
    functions: Vec<FunctionDeclaration>,
}

impl GeminiClient {
    pub fn new(api_key: String) -> Self {
        Self {
            client: reqwest::Client::new(),
            api_key,
            base_url: "https://generativelanguage.googleapis.com/v1beta/models".to_string(),
            system_prompt: None,
            functions: Vec::new(),
        }
    }

    pub fn with_system_prompt(mut self, prompt: String) -> Self {
        self.system_prompt = Some(prompt);
        self
    }

    pub fn with_functions(mut self, functions: Vec<FunctionDeclaration>) -> Self {
        self.functions = functions;
        self
    }

    pub async fn generate_content(
        &self,
        model: &str,
        contents: Vec<Content>,
    ) -> Result<GeminiAPIResponse, Box<dyn Error>> {
        let url = format!("{}/{}:generateContent", self.base_url, model);
        
        let mut request = GeminiRequest::new(contents);
        
        if let Some(ref system_prompt) = self.system_prompt {
            request = request.with_system_instruction(system_prompt.clone());
        }
        
        if !self.functions.is_empty() {
            request = request
                .with_function_declarations(self.functions.clone())
                .with_tool_config(FunctionCallingMode::Auto, None);
        }

        let response = self.client
            .post(&url)
            .header("x-goog-api-key", &self.api_key)
            .header("Content-Type", "application/json")
            .json(&request)
            .send()
            .await?;

        if !response.status().is_success() {
            let error_text = response.text().await?;
            return Err(format!("API request failed: {}", error_text).into());
        }

        let gemini_response: GeminiAPIResponse = response.json().await?;
        Ok(gemini_response)
    }

    pub async fn chat_with_functions(
        &self,
        model: &str,
        message: String,
        function_handler: impl Fn(&str, serde_json::Value) -> serde_json::Value,
    ) -> Result<String, Box<dyn Error>> {
        let mut contents = vec![Content::user(message)];
        
        loop {
            let response = self.generate_content(model, contents.clone()).await?;
            
            if response.has_function_calls() {
                for function_call in response.get_function_calls() {
                    let result = function_handler(&function_call.name, function_call.args.clone());
                    
                    contents.push(Content::model(vec![
                        crate::types::Part::function_call(function_call.name.clone(), function_call.args.clone())
                    ]));
                    contents.push(Content::function_response_content(&function_call.name, result));
                }
            } else {
                return Ok(response.get_text().unwrap_or_default());
            }
        }
    }
}


