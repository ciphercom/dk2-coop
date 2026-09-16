// The guestbook is a local joke: nothing is transmitted, stored, or rendered from user input.
const guestbook = document.querySelector('#guestbook-form');
const response = guestbook.querySelector('.guestbook-response');
guestbook.addEventListener('submit', (event) => {
  event.preventDefault();
  response.textContent = 'Your entry has been fed to the Bile Demon. Thank you for signing. He described it as “a little wordy, but surprisingly nutritious”.';
});
guestbook.addEventListener('reset', () => {
  response.textContent = 'Your conscience has been cleared. Your browser history is your own problem.';
});
// Enable the joke only after submission is intercepted, so it cannot navigate without JavaScript.
guestbook.querySelector('fieldset').disabled = false;
