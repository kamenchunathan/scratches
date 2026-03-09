use hmac::{Hmac, Mac};
use sha2::Sha256;
use worker::Headers;

const CLOCK_SKEW_SECS: u64 = 300;

// TODO: Add tests for this
pub fn verify(body: &[u8], headers: &Headers, secret: &[u8]) -> worker::Result<()> {
    let timestamp_str = headers.get("X-Timestamp")?.ok_or("Missing X-Timestamp")?;
    let signature_hex = headers.get("X-Signature")?.ok_or("Missing X-Signature")?;

    // Disallow requests that have taken too long - might be a target for replay requests
    let timestamp: u64 = timestamp_str.parse().map_err(|_| "Malformed X-Timestamp")?;
    let now = worker::Date::now().as_millis() / 1000;
    if now - timestamp > CLOCK_SKEW_SECS {
        return Err("Timestamp outside acceptable window".into());
    }

    // Confirm request sign
    let mut mac = Hmac::<Sha256>::new_from_slice(secret).expect("Unable to create HMAC");
    mac.update(timestamp_str.as_bytes());
    mac.update(b".");
    mac.update(body);

    let expected = mac.finalize().into_bytes();
    let provided = hex::decode(&signature_hex).map_err(|_| "Malformed X-Signature")?;

    if expected.as_slice() == provided.as_slice() {
        Ok(())
    } else {
        Err("Mismatched signature".into())
    }
}
