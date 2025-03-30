use serde::{de::DeserializeOwned, Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Message<Body> {
    /// Identifies the node this message came from
    pub src: String,

    /// Identifies the node this message came from
    pub dest: String,

    /// Payload of the message
    pub body: Body,
}

impl<T> Message<T> {
    pub fn map<U>(self, f: fn(T) -> U) -> Message<U> {
        let body = f(self.body);
        Message {
            src: self.src,
            dest: self.dest,
            body,
        }
    }
}

#[derive(Debug, Clone, Serialize)]
pub struct ErrorBody {
    /// `msg_id` of the request which caused this error.
    in_reply_to: u32,

    /// code is an integer which indicates the type of error which occurred.
    /// Maelstrom defines several error types, and you can also invent your own.
    /// Codes 0-999 are reserved for Maelstrom's use;
    /// codes 1000 and above are free for your own purposes.
    code: u32,

    /// optional, and may contain any explanatory message
    text: String,
}

pub trait Layer {
    type Request: DeserializeOwned;
    type Response: Serialize;

    fn handle(
        &mut self,
        node: impl NodeData,
        req: Message<Self::Request>,
    ) -> Message<Result<Self::Response, ErrorBody>>;
}

impl Layer for () {
    type Request = ();

    type Response = ();

    fn handle(
        &mut self,
        node: impl NodeData,
        req: Message<Self::Request>,
    ) -> Message<Result<Self::Response, ErrorBody>> {
        todo!()
    }
}

pub trait NodeData {
    fn node_id(&self) -> String;
    fn all_nodes(&self) -> Vec<String>;
}
