var pages = ['wifi', 'api-key', 'stations'];

function showPage(name) {
    pages.forEach(function(p) {
        document.getElementById('page-' + p).style.display = (p === name) ? '' : 'none';
    });
}

function collectConfig() {
    var stationRaw = document.getElementById('station-ids').value.trim();
    var stations = stationRaw ? stationRaw.split(',').map(function(s) { return s.trim(); }).filter(Boolean) : [];

    return {
        wifi: {
            ssid: document.getElementById('wifi-ssid').value.trim(),
            password: document.getElementById('wifi-password').value
        },
        api_key: document.getElementById('api-key').value.trim(),
        stations: stations
    };
}

function finish() {
    var config = collectConfig();
    console.log('Configured JSON:', JSON.stringify(config, null, 2));
    fetch('/api/finish', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(config)
    })
        .then(function(r) { return r.json(); })
        .then(function(data) {
            document.body.innerHTML = '<h1>Done! Device will reboot now...</h1>';
        })
        .catch(function(err) {
            alert('Error: ' + err);
        });
}
