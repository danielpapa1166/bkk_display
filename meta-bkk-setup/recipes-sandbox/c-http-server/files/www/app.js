var pages = ["wifi", "api-key", "stations"];
var navigationPending = false;

var pageValidators = {
    "wifi": function() {
        var ssid = document.getElementById("wifi-ssid").value.trim();
        if (!ssid) return "SSID is required.";
        return null;
    },
    "api-key": function() {
        var key = document.getElementById("api-key").value.trim();
        if (!key) return "API key is required.";
        return null;
    },
    "stations": function() {
        var raw = document.getElementById("station-ids").value.trim();
        if (!raw) return "At least one station ID is required.";
        return null;
    }
};

var pagePayloadCollectors = {
    "wifi": function() {
        return {
            wifi_ssid: document.getElementById("wifi-ssid").value.trim(),
            wifi_password: document.getElementById("wifi-password").value
        };
    },
    "api-key": function() {
        return { api_key: document.getElementById("api-key").value.trim() };
    },
    "stations": function() {
        return { station_ids: document.getElementById("station-ids").value.trim() };
    }
};

function getCurrentPageName() {
    for (var i = 0; i < pages.length; i++) {
        var pageName = pages[i];
        var pageEl = document.getElementById("page-" + pageName);
        if (pageEl && pageEl.style.display !== "none") {
            return pageName;
        }
    }
    return null;
}

function setNavigationButtonsDisabled(disabled) {
    var actionButtons = document.querySelectorAll(".actions button");
    for (var i = 0; i < actionButtons.length; i++) {
        actionButtons[i].disabled = disabled;
    }
}

function parseJsonSafely(text) {
    try {
        return JSON.parse(text);
    } catch (err) {
        return null;
    }
}

function isBackendOkPayload(text) {
    var trimmed = (text || "").trim().toLowerCase();
    if (trimmed === "ok") {
        return true;
    }

    var data = parseJsonSafely(text);
    return !!(data && data.status === "ok");
}

function sendButtonPress(targetPage, extraData) {
    var currentPage = getCurrentPageName();
    var fromIndex = pages.indexOf(currentPage);
    var toIndex = pages.indexOf(targetPage);
    var direction = "jump";

    if (fromIndex !== -1 && toIndex !== -1) {
        direction = toIndex > fromIndex ? "next" : "back";
    }

    var payload = Object.assign({
        action: direction,
        from_page: currentPage,
        to_page: targetPage
    }, extraData || {});

    return fetch("/api/button", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload)
    })
        .then(function(response) {
            return response.text().then(function(text) {
                if (!response.ok) {
                    throw new Error("Backend rejected button press (HTTP " + response.status + ")");
                }

                if (!isBackendOkPayload(text)) {
                    throw new Error("Backend response was not ok");
                }
            });
        });
}

function showPage(name) {
    if (navigationPending) {
        return;
    }

    var currentPage = getCurrentPageName();
    var fromIndex = pages.indexOf(currentPage);
    var toIndex   = pages.indexOf(name);
    var isForward = toIndex > fromIndex;

    if (isForward && pageValidators[currentPage]) {
        var error = pageValidators[currentPage]();
        if (error) { alert(error); return; }
    }

    var extraData = pagePayloadCollectors[currentPage] ? pagePayloadCollectors[currentPage]() : {};

    navigationPending = true;
    setNavigationButtonsDisabled(true);

    sendButtonPress(name, extraData)
        .then(function() {
            pages.forEach(function(p) {
                document.getElementById("page-" + p).style.display = (p === name) ? "" : "none";
            });
        })
        .catch(function(err) {
            alert("Could not change page: " + err.message);
        })
        .finally(function() {
            navigationPending = false;
            setNavigationButtonsDisabled(false);
        });
}

function collectConfig() {
    var stationRaw = document.getElementById("station-ids").value.trim();
    var stations = stationRaw
        ? stationRaw.split(",").map(function(s) { return s.trim(); }).filter(Boolean)
        : [];

    return {
        wifi: {
            ssid: document.getElementById("wifi-ssid").value.trim(),
            password: document.getElementById("wifi-password").value
        },
        api_key: document.getElementById("api-key").value.trim(),
        stations: stations
    };
}

function finish() {
    var config = collectConfig();
    console.log("Configured JSON:", JSON.stringify(config, null, 2));

    fetch("/api/finish", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(config)
    })
        .then(function(r) { return r.json(); })
        .then(function() {
            document.body.innerHTML = "<h1>Done! Device will reboot now...</h1>";
        })
        .catch(function(err) {
            alert("Error: " + err);
        });
}
