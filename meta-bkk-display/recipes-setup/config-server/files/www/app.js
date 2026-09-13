/* ==========================================================================
   BKK Display setup frontend
   Phase 1 (--mode=wifi):  WiFi credentials -> Apply & Reboot
   Phase 2 (--mode=api):   API key + station IDs -> Finish
   ========================================================================== */

var currentPhase = null;
var navigationPending = false;

/* ---------- helpers ------------------------------------------------------- */

function showOnlyPage(name) {
    var all = ["loading", "wifi", "api", "done"];
    all.forEach(function(p) {
        var el = document.getElementById("page-" + p);
        if (el) el.style.display = (p === name) ? "" : "none";
    });
}

function setStatus(msg, isError) {
    var el = document.getElementById("status-msg");
    el.textContent = msg;
    el.className = "status" + (isError ? " error" : "");
    el.style.display = msg ? "" : "none";
}

function setButtonsDisabled(disabled) {
    document.querySelectorAll(".actions button").forEach(function(b) {
        b.disabled = disabled;
    });
}

function isOkResponse(text) {
    var t = (text || "").trim().toLowerCase();
    if (t === "ok") return true;
    try { var d = JSON.parse(text); return !!(d && d.status === "ok"); }
    catch (e) { return false; }
}

function postJson(endpoint, payload) {
    return fetch(endpoint, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload)
    }).then(function(r) {
        return r.text().then(function(text) {
            if (!r.ok) throw new Error("Server error (HTTP " + r.status + ")");
            if (!isOkResponse(text)) throw new Error("Unexpected server response");
        });
    });
}

/* ---------- validators ---------------------------------------------------- */

function validateWifi() {
    var ssid = document.getElementById("wifi-ssid").value.trim();
    if (!ssid) return "SSID is required.";
    return null;
}

function validateApiSetup() {
    var key = document.getElementById("api-key").value.trim();
    if (!key) return "API key is required.";

    var raw = document.getElementById("station-ids").value.trim();
    if (!raw) return "At least one station ID is required.";

    return null;
}

/* ---------- station search (autocomplete) ---------------------------------- */

var STATION_SEARCH_MIN_CHARS = 4;
var STATION_SEARCH_DEBOUNCE_MS = 300;
var stationSearchTimer = null;

function clearStationResults() {
    var list = document.getElementById("station-search-results");
    list.innerHTML = "";
    list.style.display = "none";
}

function addStationId(stopId) {
    var textarea = document.getElementById("station-ids");
    var existing = textarea.value.split(/[\s,]+/).filter(function(s) { return s.length > 0; });
    if (existing.indexOf(stopId) === -1) {
        existing.push(stopId);
    }
    textarea.value = existing.join(", ");
}

function renderStationResults(stations) {
    var list = document.getElementById("station-search-results");
    list.innerHTML = "";

    if (!stations || stations.length === 0) {
        var empty = document.createElement("li");
        empty.className = "search-result-empty";
        empty.textContent = "No matching stations";
        list.appendChild(empty);
        list.style.display = "";
        return;
    }

    stations.forEach(function(station) {
        var item = document.createElement("li");
        item.className = "search-result-item";
        item.textContent = station.stop_name + " (" + station.stop_id + ")";
        item.addEventListener("click", function() {
            addStationId(station.stop_id);
            document.getElementById("station-search").value = "";
            clearStationResults();
        });
        list.appendChild(item);
    });
    list.style.display = "";
}

function searchStations(query) {
    return fetch("/api/stations/search", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ query: query })
    }).then(function(r) {
        if (!r.ok) throw new Error("HTTP " + r.status);
        return r.json();
    });
}

function onStationSearchInput() {
    var query = document.getElementById("station-search").value.trim();

    if (stationSearchTimer) {
        clearTimeout(stationSearchTimer);
        stationSearchTimer = null;
    }

    if (query.length < STATION_SEARCH_MIN_CHARS) {
        clearStationResults();
        return;
    }

    stationSearchTimer = setTimeout(function() {
        searchStations(query)
            .then(function(data) {
                renderStationResults(data.stations);
            })
            .catch(function() {
                clearStationResults();
            });
    }, STATION_SEARCH_DEBOUNCE_MS);
}

/* ---------- phase 1: apply wifi & reboot ---------------------------------- */

function applyWifi() {
    if (navigationPending) return;

    var err = validateWifi();
    if (err) { setStatus(err, true); return; }

    setStatus("");
    navigationPending = true;
    setButtonsDisabled(true);

    var payload = {
        action: "next",
        from_page: "wifi",
        to_page: "done",
        wifi_ssid: document.getElementById("wifi-ssid").value.trim(),
        wifi_password: document.getElementById("wifi-password").value
    };

    postJson("/api/button", payload)
        .then(function() {
            document.getElementById("done-title").textContent = "WiFi Saved";
            document.getElementById("done-message").textContent =
                "Credentials saved. The device will reboot and connect to your network.";
            showOnlyPage("done");
        })
        .catch(function(err) {
            setStatus(err.message, true);
            navigationPending = false;
            setButtonsDisabled(false);
        });
    /* buttons stay disabled after success — reboot is imminent */
}

/* ---------- phase 2: save API key + stations ------------------------------ */

function finishApi() {
    if (navigationPending) return;

    var err = validateApiSetup();
    if (err) { setStatus(err, true); return; }

    setStatus("");
    navigationPending = true;
    setButtonsDisabled(true);

    var payload = {
        action: "next",
        from_page: "api",
        to_page: "done",
        api_key: document.getElementById("api-key").value.trim(),
        station_ids: document.getElementById("station-ids").value.trim()
    };

    postJson("/api/button", payload)
        .then(function() {
            document.getElementById("done-title").textContent = "Setup Complete";
            document.getElementById("done-message").textContent =
                "Configuration saved. The display app will start automatically.";
            showOnlyPage("done");
        })
        .catch(function(err) {
            setStatus(err.message, true);
            navigationPending = false;
            setButtonsDisabled(false);
        });
}

/* ---------- init: fetch mode and show first page -------------------------- */

document.addEventListener("DOMContentLoaded", function() {
    var searchInput = document.getElementById("station-search");
    if (searchInput) {
        searchInput.addEventListener("input", onStationSearchInput);
    }

    document.addEventListener("click", function(evt) {
        var results = document.getElementById("station-search-results");
        if (!results || results.style.display === "none") return;
        if (evt.target === searchInput || results.contains(evt.target)) return;
        clearStationResults();
    });

    fetch("/api/mode")
        .then(function(r) {
            if (!r.ok) throw new Error("HTTP " + r.status);
            return r.json();
        })
        .then(function(data) {
            currentPhase = data.mode;
            if (currentPhase === "access_point") {
                showOnlyPage("wifi");
            } else if (currentPhase === "wifi_client") {
                showOnlyPage("api");
            } else {
                throw new Error("Unknown mode: " + currentPhase);
            }
        })
        .catch(function(err) {
            setStatus("Could not load setup mode: " + err.message, true);
        });
});