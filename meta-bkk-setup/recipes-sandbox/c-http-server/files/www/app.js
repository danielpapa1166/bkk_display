document.addEventListener('DOMContentLoaded', function() {
    const button = document.getElementById('myButton');
    button.addEventListener('click', async () => {
    try {
        console.log('Button clicked, sending request to server...');

        const response = await fetch('/api/button', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ action: 'button_pressed', timestamp: Date.now() })
        });

        if (!response.ok) {
            const errorText = await response.text();
            throw new Error(`HTTP ${response.status}: ${errorText}`);
        }

        const contentType = response.headers.get('content-type') || '';
        if (!contentType.includes('application/json')) {
            const payload = await response.text();
            throw new Error(`Expected JSON response, received: ${payload}`);
        }

        const data = await response.json();
        console.log('Server response:', data);
    } catch (error) {
        console.error('Error:', error);
    }
    });
});
