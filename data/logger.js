var proto = window.location.protocol === 'https:' ? 'wss' : 'ws';
var socket = new WebSocket(`${proto}://${window.location.host}/ws`);

var logger = document.getElementById('log');
var messageCount = 0;

socket.onopen = function (_event) {
  document.getElementById('commandInput').removeAttribute('disabled');
  document.getElementById('sendCommand').removeAttribute('disabled');
  logger.innerHTML += `\nWebSocket Connected!\n`
};

socket.onmessage = function(event) {
  var message = event.data;
  if (messageCount >= 100) {
    logger.removeChild(logger.firstChild);
    messageCount = 0;
  }
  logger.innerHTML += `< ${message}\n`;
  logger.scrollTop = logger.scrollHeight;
  messageCount++;
};

document.getElementById('reboot').addEventListener('click', () => {
  fetch('reboot')
    .then(() => setTimeout(() => location.reload(), 5000))
    .catch(() => setTimeout(() => location.reload(), 5000));
});

function sendCommand() {
  var command = document.getElementById('commandInput').value;
  if (command && socket.readyState === socket.OPEN) {
    logger.innerHTML += `> ${command}\n`;
    socket.send(command);
    document.getElementById('commandInput').value = '';
    logger.scrollTop = logger.scrollHeight;
    messageCount++;
  } else {
    console.log('Socket not open.')
  }
}

document.getElementById('sendCommand').addEventListener('click', sendCommand);

document.getElementById('commandInput').addEventListener('keypress', (event) => {
  if (event.key === 'Enter') {
    sendCommand();
  }
});
