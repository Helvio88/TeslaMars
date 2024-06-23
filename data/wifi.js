document.addEventListener("DOMContentLoaded", function() {
  fetch('/wifi')
    .then(response => response.json())
    .then(data => {
      // Sort by RSSI in descending order and remove duplicates
      const uniqueSsids = [];
      const ssidSet = new Set();
      data.sort((a, b) => b.rssi - a.rssi).forEach(item => {
        if (!ssidSet.has(item.ssid)) {
          ssidSet.add(item.ssid);
          uniqueSsids.push(item.ssid);
        }
      });

      // Populate the dropdown
      const ssidSelect = document.getElementById('ssid');
      ssidSelect.innerHTML = '';
      uniqueSsids.forEach(ssid => {
        const option = document.createElement('option');
        option.value = ssid;
        option.textContent = ssid;
        ssidSelect.appendChild(option);
        ssidSelect.removeAttribute('disabled');
      });
    })
    .catch(error => {
      console.error('Error fetching WiFi data:', error);
    });

  // Add event listener for factory reset button
  document.getElementById('factory-reset').addEventListener('click', function() {
    if (confirm('Are you sure you want to factory reset?')) {
      fetch('/factory')
        .then(response => {
          if (response.ok) {
            alert('Factory reset successful.');
          } else {
            alert('Factory reset failed.');
          }
        })
        .catch(error => {
          console.error('Error during factory reset:', error);
          alert('Factory reset failed.');
        });
    }
  });
});
