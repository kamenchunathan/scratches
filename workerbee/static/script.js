const joinForm = document.getElementById("join-form");
const roomNameInput = document.getElementById("room-name-input");
const roomSelectionDiv = document.getElementById("room-selection");
const chatRoomDiv = document.getElementById("chat-room");
const roomTitle = document.getElementById("room-title");
const messagesDiv = document.getElementById("messages");
const messageForm = document.getElementById("message-form");
const messageInput = document.getElementById("message-input");

let ws;
let roomName;

joinForm.addEventListener("submit", (e) => {
    e.preventDefault();
    roomName = roomNameInput.value.trim();
    if (roomName) {
        connectWebSocket(roomName);
        roomSelectionDiv.style.display = "none";
        chatRoomDiv.style.display = "block";
        roomTitle.textContent = `Chatroom: ${roomName}`;
    }
});

messageForm.addEventListener("submit", (e) => {
    e.preventDefault();
    const message = messageInput.value.trim();
    if (message && ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({ type: "message", payload: message }));
        messageInput.value = "";
    }
});

function connectWebSocket(room) {
    // Use ws:// for http and wss:// for https
    const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
    ws = new WebSocket(`${protocol}//${window.location.host}/ws/${room}`);

    ws.onopen = () => {
        console.log("WebSocket connected");
        appendMessage("System", `Joined room: ${room}`);
    };

    ws.onmessage = (event) => {
        const data = JSON.parse(event.data);
        if (data.type === "message") {
            appendMessage(data.sender, data.payload);
        }
    };

    ws.onclose = () => {
        console.log("WebSocket disconnected");
        appendMessage(
            "System",
            "Disconnected from chatroom. Please refresh to rejoin.",
        );
    };

    ws.onerror = (error) => {
        console.error("WebSocket error:", error);
        appendMessage("System", "WebSocket error occurred.");
    };
}

function appendMessage(sender, message) {
    const messageElement = document.createElement("div");
    messageElement.classList.add("message");
    messageElement.innerHTML = `<strong>${sender}:</strong> ${message}`;
    messagesDiv.appendChild(messageElement);
    messagesDiv.scrollTop = messagesDiv.scrollHeight;
}
