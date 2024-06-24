var proto = window.location.protocol === 'http:' ? 'ws' : 'wss';
var ws = new WebSocket(`${proto}://${window.location.host}/ws`);

// DOM
var logger = document.getElementById('log');
var cmd = document.getElementById('commandInput');
var btn = document.getElementById('sendCommand');
var i = 2;

ws.onopen = () => {
  [cmd, btn].forEach(el => el.removeAttribute('disabled'));
  logger.innerHTML += `<div>WebSocket Connected!</div>`
};

ws.onmessage = (e) => {
  console.log(e.data);
  var twai = JSON.parse(e.data);
  var data = twai.data.map(num => num.toString().padStart(3, ' ')).join(' | ');
  const el = document.createElement('div');
  el.innerHTML = `(<a href="#" onclick="ws.send(${twai.id})">0x${twai.id.toString(16).padStart(3, '0')}</a>): ${data}`;
  if (i++ > 100) logger.removeChild(logger.firstChild);
  logger.append(el);
  logger.scrollTop = logger.scrollHeight;
};

sendCommand = () => {
  if (cmd.value && ws.readyState === ws.OPEN) {
    logger.innerHTML += `> ${cmd.value}\n`;
    ws.send(cmd.value);
    logger.scrollTop = logger.scrollHeight;
    i++;
  } else {
    logger.innerHTML = 'Socket Closed. Please refresh the page.';
  }
  cmd.value = '';
}

btn.addEventListener('click', sendCommand);
cmd.addEventListener('keypress', (e) => e.key === 'Enter' && sendCommand());
