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
        const data = await response.json();
        console.log('Server response:', data);
    } catch (error) {
        console.error('Error:', error);
    }
    });
});
