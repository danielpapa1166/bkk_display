'use strict';

const STATUS_POLL_MS = 2000;

const tbody = document.getElementById('app-tbody');
const lastUpdated = document.getElementById('last-updated');

function badgeClass(status) {
  return 'badge badge-' + status.toLowerCase().replace(/ /g, '_');
}

function renderRows(apps) {
  if (!apps || apps.length === 0) {
    tbody.innerHTML = '<tr><td colspan="4" class="loading">No applications.</td></tr>';
    return;
  }

  const rows = apps.map(app => {
    const pid = app.pid > 0 ? app.pid : '—';
    const exitCode = (app.status === 'EXITED' || app.status === 'FAILED') ? app.exit_code : '—';
    return `<tr>
      <td class="name">${escHtml(app.name)}</td>
      <td class="pid">${pid}</td>
      <td><span class="${badgeClass(app.status)}">${escHtml(app.status)}</span></td>
      <td class="exit-code">${exitCode}</td>
    </tr>`;
  });

  tbody.innerHTML = rows.join('');
}

function escHtml(str) {
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

async function poll() {
  document.body.classList.add('refreshing');

  try {
    const resp = await fetch('/status');
    if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
    const data = await resp.json();
    renderRows(data.apps);
    lastUpdated.textContent = 'Updated ' + new Date().toLocaleTimeString();
  } catch (err) {
    lastUpdated.textContent = 'Error: ' + err.message;
  } finally {
    document.body.classList.remove('refreshing');
  }
}

poll();
setInterval(poll, STATUS_POLL_MS);
