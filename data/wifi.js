document.addEventListener("DOMContentLoaded", async () => {
  var res = await fetch('/wifi');
  var data = await res.json();
  
  // Sort by RSSI in descending order and remove duplicates
  const ssids = [];
  data.sort((a, b) => b.rssi - a.rssi).forEach(ssid => {
    if (!ssids.find(x => x === ssid.ssid))
      ssids.push(ssid.ssid);
  });

  // Populate the dropdown
  const sel = document.getElementById('ssid');
  sel.innerHTML = '';
  ssids.forEach(ssid => {
    const option = document.createElement('option');
    option.value = ssid;
    option.textContent = ssid;
    sel.appendChild(option);
  });
  sel.removeAttribute('disabled');
});

var reset = async () => {
  if (confirm('Are you sure you want to factory reset Tesla Mars?')) {
    var res = await fetch('/factory');
    if (res.ok)
      alert('Device will reboot into factory settings.');
    else
      alert('Something went wrong. Please try again.');
  }
};
