var pages = ['wifi', 'api-key', 'stations'];

function showPage(name) {
    pages.forEach(function(p) {
        document.getElementById('page-' + p).style.display = (p === name) ? '' : 'none';
    });
}

function finish() {
    fetch('/api/finish', { method: 'POST' })
        .then(function(r) { return r.json(); })
        .then(function(data) {
            document.body.innerHTML = '<h1>Done! Device will reboot now...</h1>';
        })
        .catch(function(err) {
            alert('Error: ' + err);
        });
}
