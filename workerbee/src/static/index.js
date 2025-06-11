document.addEventListener("DOMContentLoaded", () => {
  const WEBSOCKET_URL = "ws://localhost:8787/v1/counter";

  const statusDisplay = document.getElementById("status-display");
  const counterValue = document.getElementById("counter-value");
  const incrementButton = document.getElementById("increment-button");

  let socket;

  function connectWebSocket() {
    console.log("Attempting to connect to WebSocket...");
    updateStatus("Connecting...", "status-connecting");
    incrementButton.disabled = true;

    socket = new WebSocket(WEBSOCKET_URL);

    // Event listener for when the connection is opened
    socket.onopen = (_) => {
      console.log("WebSocket connection established.");
      updateStatus("Connected", "status-open");
      incrementButton.disabled = false;
    };

    // Event listener for receiving messages from the server
    socket.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        console.log("Message received:", data);

        // Expecting a message like: { "type": "update", "value": 123 }
        if (data.type === "update" && typeof data.value === "number") {
          counterValue.textContent = data.value;
        }
      } catch (error) {
        console.error("Error parsing message from server:", error);
      }
    };

    // Event listener for when the connection is closed
    socket.onclose = (event) => {
      console.warn("WebSocket connection closed.", event);
      updateStatus("Disconnected", "status-closed");
      incrementButton.disabled = true;
      // Optional: Attempt to reconnect after a delay
      setTimeout(connectWebSocket, 3000); // Reconnect after 3 seconds
    };

    // Event listener for any errors
    socket.onerror = (error) => {
      console.error("WebSocket error:", error);
      updateStatus("Connection Error", "status-closed");
      socket.close(); // Ensure the socket is closed on error
    };
  }

  // Function to update the status display
  function updateStatus(message, className) {
    statusDisplay.textContent = message;
    statusDisplay.className = className; // Resets class list
  }

  // Function to send an increment message
  function sendIncrementMessage() {
    if (socket && socket.readyState === WebSocket.OPEN) {
      const message = {
        action: "increment",
      };
      socket.send(JSON.stringify(message));
      console.log("Sent 'increment' message.");
    } else {
      console.error("Cannot send message: WebSocket is not open.");
    }
  }

  // --- Initial Setup ---
  incrementButton.addEventListener("click", sendIncrementMessage);
  connectWebSocket();
});
