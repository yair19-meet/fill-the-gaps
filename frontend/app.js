// Game State & DOM Elements
let score = 0;
let timeLeft = 120;
let timerInterval = null;
let currentBrokenWord = [];
let correctWords = [];

// Wasm State
let gameEngine;
let WasmModule;

// Screens
const screenLanding = document.getElementById('screen-landing');
const screenGameplay = document.getElementById('screen-gameplay');
const screenGameOver = document.getElementById('screen-gameover');
const screenAbout = document.getElementById('screen-about');

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
const sidebarToggle = document.getElementById('sidebar-toggle');
const sidebar = document.getElementById('sidebar');
const sidebarWords = document.getElementById('sidebar-words');
const navAbout = document.getElementById('nav-about');
const mainTitle = document.querySelector('.main-title');
const btnSummary = document.getElementById('btn-summary');
const summaryContainer = document.getElementById('summary-container');
const summaryList = document.getElementById('summary-list');

// Event Listeners
btnStart.addEventListener('click', startGame);
btnRestart.addEventListener('click', startGame);
guessForm.addEventListener('submit', handleGuess);
btnSummary.addEventListener('click', toggleSummary);
sidebarToggle.addEventListener('click', () => {
    sidebar.classList.toggle('collapsed');
});
navAbout.addEventListener('click', () => {
    showScreen(screenAbout);
    clearInterval(timerInterval);
});
mainTitle.addEventListener('click', () => {
    showScreen(screenLanding);
    clearInterval(timerInterval);
});
mainTitle.style.cursor = 'pointer';

// Initialize Wasm Module
async function initWasm() {
    try {
        WasmModule = await createGameModule();
        gameEngine = new WasmModule.Operation();
        
        // Emscripten's virtual filesystem has the preloaded data/ folder
        gameEngine.LoadDictionary("data/dictionary_rich.txt");
        
        // Hide loading overlay
        const loadingOverlay = document.getElementById('loading-overlay');
        if (loadingOverlay) {
            loadingOverlay.style.opacity = '0';
            setTimeout(() => loadingOverlay.remove(), 500);
        }
        console.log("Game Engine loaded successfully.");
    } catch (err) {
        console.error("Failed to initialize Wasm:", err);
        const loadingText = document.querySelector('#loading-overlay p');
        if (loadingText) loadingText.textContent = "Error loading game engine. Please refresh.";
    }
}

initWasm();



// Interactive Theme Selection
// themeCheckboxes.forEach(cb => {
//     cb.addEventListener('change', async () => {
//         const selectedThemes = Array.from(document.querySelectorAll('input[name="theme"]:checked')).map(cb => cb.value);
//         if (selectedThemes.length === 0) {
//             // Prevent unchecking all
//             cb.checked = true;
//             return;
//         }
//         // Immediately set themes on the backend
//         try {
//             const themesParam = selectedThemes.join(',');
//             await fetch(`/api/set_themes?themes=${themesParam}`);
//         } catch (err) {
//             console.error("Error setting themes dynamically:", err);
//         }
//     });
// });

// Functions
function showScreen(screenElement) {
    document.querySelectorAll('.screen').forEach(s => s.classList.remove('active'));
    screenElement.classList.add('active');
}

async function startGame(e) {
    // const selectedThemes = Array.from(document.querySelectorAll('input[name="theme"]:checked')).map(cb => cb.value);
    // if (selectedThemes.length === 0) {
    //     alert('Please select at least one theme!');
    //     return;
    // }
    // 
    // // Set themes on the backend
    // try {
    //     const themesParam = selectedThemes.join(',');
    //     await fetch(`/api/set_themes?themes=${themesParam}`);
    // } catch (err) {
    //     console.error("Error setting themes:", err);
    // }

    score = 0;
    timeLeft = 120;
    correctWords = [];
    summaryContainer.style.display = 'none';
    // Clear sidebar word list
    sidebarWords.innerHTML = '<p class="sidebar-empty-msg">Words you guess correctly will appear here!</p>';
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

    // Prepare summary
    summaryList.innerHTML = '';
    if (correctWords.length === 0) {
        summaryList.innerHTML = '<p style="color: var(--text-muted); font-style: italic;">No words yet. Try again!</p>';
    } else {
        correctWords.forEach(word => {
            const span = document.createElement('span');
            span.className = 'summary-word';
            span.textContent = word;
            summaryList.appendChild(span);
        });
    }

    showScreen(screenGameOver);
}

function toggleSummary() {
    if (summaryContainer.style.display === 'none') {
        summaryContainer.style.display = 'block';
        btnSummary.textContent = 'HIDE SUMMARY';
    } else {
        summaryContainer.style.display = 'none';
        btnSummary.textContent = 'SUMMARY';
    }
}

function updateScore() {
    scoreDisplay.textContent = score;
}

function recordCorrectWord(word) {
    // Single source of truth: update array and sidebar together
    correctWords.push(word);

    const emptyMsg = sidebarWords.querySelector('.sidebar-empty-msg');
    if (emptyMsg) emptyMsg.remove();

    const entry = document.createElement('div');
    entry.className = 'sidebar-word-entry';
    entry.innerHTML = `<span class="sidebar-word-check">✓</span><span class="sidebar-word-text">${word}</span>`;
    sidebarWords.prepend(entry);

    requestAnimationFrame(() => entry.classList.add('visible'));
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
    if (!gameEngine) return [];
    try {
        const brokenWordVector = gameEngine.GenerateBrokenWord();
        // Convert Wasm StringVector to JS Array
        const brokenWord = [];
        for (let i = 0; i < brokenWordVector.size(); i++) {
            brokenWord.push(brokenWordVector.get(i));
        }
        return brokenWord;
    } catch (err) {
        console.error("Error generating word:", err);
        return [];
    }
}

async function checkWordWithBackend(guess, brokenWord) {
    if (!gameEngine) return { valid: false };
    try {
        // Convert JS Array to Wasm StringVector
        const brokenWordVector = new WasmModule.StringVector();
        brokenWord.forEach(part => brokenWordVector.push_back(part));

        const result = gameEngine.checkWordValidity(guess, brokenWordVector);
        
        // Cleanup vector to avoid memory leaks
        brokenWordVector.delete();
        
        return {
            valid: result.valid,
            correctWord: result.correctWord
        };
    } catch (err) {
        console.error("Error checking word:", err);
        return { valid: false };
    }
}

async function nextWord() {
    currentBrokenWord = await fetchNextWord();
    if (currentBrokenWord && currentBrokenWord.length > 0) {
        renderWord(currentBrokenWord);
    } else {
        wordDisplay.innerHTML = '<span class="text-orange" style="font-size: 1.5rem">Error loading word. Please refresh.</span>';
    }
    wordInput.value = '';
}

function renderWord(brokenWordParts) {
    wordDisplay.innerHTML = '';
    // The C++ engine returns an array of string parts (e.g. ["p", "l", "t", "cs"])
    // We join them with '*' to insert the implicit gaps between the parts.
    const fullString = brokenWordParts.join('*');
    const chars = fullString.split('');

    // Dynamically scale font size and gap for longer words to fit the 420px oval
    let fontSize = 2.5; // default 2.5rem
    let gap = 8;        // default 8px
    if (chars.length > 9) {
        const scaleFactor = 9 / chars.length;
        // ensure it doesn't get too small
        fontSize = Math.max(1.2, 2.5 * scaleFactor);
        gap = Math.max(3, 8 * scaleFactor);
    }
    wordDisplay.style.fontSize = `${fontSize}rem`;
    wordDisplay.style.gap = `${gap}px`;

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

function showFeedback(isSuccess, correctWord = '') {
    feedbackMessage.textContent = isSuccess ? 'WELL DONE!' : `INCORRECT. The word was: ${correctWord}`;
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

    const result = await checkWordWithBackend(guess, currentBrokenWord);

    if (result.valid) {
        wordInput.disabled = false;
        wordInput.focus();
        score++;
        recordCorrectWord(guess);
        updateScore();
        showFeedback(true);
        // Add a nice pop effect to the score
        scoreDisplay.parentElement.style.transform = 'scale(1.3)';
        setTimeout(() => scoreDisplay.parentElement.style.transform = 'scale(1)', 200);

        await nextWord();
    } else {
        showFeedback(false, result.correctWord || '');
        setTimeout(async () => {
            wordInput.disabled = false;
            wordInput.focus();
            await nextWord();
        }, 1500);
    }
}
