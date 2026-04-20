// Game State & DOM Elements
let score = 0;
let timeLeft = 120;
let timerInterval = null;
let currentBrokenWord = [];

// Screens
const screenLanding = document.getElementById('screen-landing');
const screenGameplay = document.getElementById('screen-gameplay');
const screenGameOver = document.getElementById('screen-gameover');

// Elements
const btnStart = document.getElementById('btn-start');
const btnRestart = document.getElementById('btn-restart');
const timerDisplay = document.getElementById('timer');
const scoreDisplay = document.getElementById('score');
const finalScoreDisplay = document.getElementById('final-score-value');
const wordDisplay = document.getElementById('word-display');
const guessForm = document.getElementById('guess-form');
const wordInput = document.getElementById('word-input');
const feedbackMessage = document.getElementById('feedback-message');

// Event Listeners
btnStart.addEventListener('click', startGame);
btnRestart.addEventListener('click', startGame);
guessForm.addEventListener('submit', handleGuess);

// Functions
function showScreen(screenElement) {
    document.querySelectorAll('.screen').forEach(s => s.classList.remove('active'));
    screenElement.classList.add('active');
}

async function startGame() {
    score = 0;
    timeLeft = 120;
    updateScore();
    updateTimer();

    showScreen(screenGameplay);
    wordInput.value = '';
    wordInput.focus();

    await nextWord();

    clearInterval(timerInterval);
    timerInterval = setInterval(() => {
        timeLeft--;
        updateTimer();

        if (timeLeft <= 0) {
            endGame();
        }
    }, 1000);
}

function endGame() {
    clearInterval(timerInterval);
    finalScoreDisplay.textContent = score;
    showScreen(screenGameOver);
}

function updateScore() {
    scoreDisplay.textContent = score;
}

function updateTimer() {
    timerDisplay.textContent = timeLeft;
    if (timeLeft <= 10) {
        timerDisplay.parentElement.style.color = 'var(--error)';
        timerDisplay.parentElement.style.animation = 'shake 1s infinite';
    } else {
        timerDisplay.parentElement.style.color = 'var(--accent-light)';
        timerDisplay.parentElement.style.animation = 'none';
    }
}

async function fetchNextWord() {
    try {
        const res = await fetch('/api/generate');
        const data = await res.json();
        return data.brokenWord;
    } catch (err) {
        console.error("Error fetching word:", err);
        return [];
    }
}

async function checkWordWithBackend(guess, brokenWord) {
    try {
        const brokenParam = brokenWord.join(',');
        const res = await fetch(`/api/check?guess=${encodeURIComponent(guess)}&broken=${encodeURIComponent(brokenParam)}`);
        const data = await res.json();
        return data.valid;
    } catch (err) {
        console.error("Error checking word:", err);
        return false;
    }
}

async function nextWord() {
    currentBrokenWord = await fetchNextWord();
    if (currentBrokenWord && currentBrokenWord.length > 0) {
        renderWord(currentBrokenWord);
    } else {
        wordDisplay.innerHTML = '<span class="text-orange" style="font-size: 1.5rem">Error loading word. Ensure C++ server is running.</span>';
    }
    wordInput.value = '';
}

function renderWord(brokenWordParts) {
    wordDisplay.innerHTML = '';
    // The C++ engine returns an array of string parts (e.g. ["p", "l", "t", "cs"])
    // We join them with '*' to insert the implicit gaps between the parts.
    const fullString = brokenWordParts.join('*');
    const chars = fullString.split('');

    chars.forEach((char, index) => {
        const span = document.createElement('span');
        span.classList.add('letter');
        if (char === '*') {
            span.classList.add('gap');
            span.textContent = '*';
        } else {
            span.textContent = char;
        }
        span.style.animationDelay = `${index * 0.05}s`;
        wordDisplay.appendChild(span);
    });
}

function showFeedback(isSuccess) {
    feedbackMessage.textContent = isSuccess ? 'WELL DONE!' : 'INCORRECT';
    feedbackMessage.className = `feedback-message ${isSuccess ? 'feedback-success' : 'feedback-error'}`;

    setTimeout(() => {
        feedbackMessage.className = 'feedback-message';
    }, 1500);
}

async function handleGuess(e) {
    e.preventDefault();
    const guess = wordInput.value.trim();
    if (!guess) return;

    // Disable input while checking
    wordInput.disabled = true;

    const isValid = await checkWordWithBackend(guess, currentBrokenWord);

    wordInput.disabled = false;
    wordInput.focus();

    if (isValid) {
        score++;
        updateScore();
        showFeedback(true);
        // Add a nice pop effect to the score
        scoreDisplay.parentElement.style.transform = 'scale(1.3)';
        setTimeout(() => scoreDisplay.parentElement.style.transform = 'scale(1)', 200);
        
        await nextWord();
    } else {
        showFeedback(false);
        await nextWord();
    }
}
