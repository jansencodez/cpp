// C++ Server Development Course - Interactive Features

document.addEventListener("DOMContentLoaded", function () {
  console.log("🚀 C++ Server Development Course loaded!");

  // Initialize interactive features
  initializeCodeEditors();
  initializeProgressTracking();
  initializeNavigation();

  // Add smooth scrolling for anchor links
  document.querySelectorAll('a[href^="#"]').forEach((anchor) => {
    anchor.addEventListener("click", function (e) {
      e.preventDefault();
      const target = document.querySelector(this.getAttribute("href"));
      if (target) {
        target.scrollIntoView({
          behavior: "smooth",
          block: "start",
        });
      }
    });
  });
});

// Initialize interactive code editors
function initializeCodeEditors() {
  const codeEditors = document.querySelectorAll(".code-editor");

  codeEditors.forEach((editor) => {
    const textarea = editor.querySelector(".code-input");
    const output = editor.querySelector(".code-output");
    const runButton = editor.querySelector(".btn-primary");
    const resetButton = editor.querySelector(".btn-secondary");

    if (runButton) {
      runButton.addEventListener("click", () => runCode(textarea, output));
    }

    if (resetButton) {
      resetButton.addEventListener("click", () => resetCode(textarea, output));
    }

    // Add syntax highlighting
    if (textarea) {
      textarea.addEventListener("input", () => {
        highlightSyntax(textarea);
      });
    }
  });
}

// Run code in the editor
function runCode(textarea, output) {
  if (!textarea || !output) return;

  const code = textarea.value;

  // Simple C++ code validation (basic)
  if (code.includes("int main()") || code.includes("std::cout")) {
    output.textContent =
      "✅ Code looks like valid C++!\n\nNote: This is a demo. In a real environment, you would compile and run this code.";
    output.style.color = "#68d391";
  } else {
    output.textContent =
      "⚠️  This doesn't look like C++ code. Try adding some C++ syntax like #include <iostream> or int main().";
    output.style.color = "#f6ad55";
  }

  // Show output
  output.style.display = "block";
}

// Reset code in the editor
function resetCode(textarea, output) {
  if (!textarea || !output) return;

  // Reset to default code based on context
  const defaultCode = getDefaultCode(textarea);
  textarea.value = defaultCode;
  output.textContent = "";
  output.style.display = "none";

  // Re-highlight syntax
  highlightSyntax(textarea);
}

// Get default code based on context
function getDefaultCode(textarea) {
  const context = textarea.closest(".code-editor");
  if (context && context.textContent.includes("socket")) {
    return `int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
if (serverSocket == -1) {
    std::cerr << "Failed to create socket" << std::endl;
    return false;
}`;
  }

  return `// Enter your C++ code here
#include <iostream>

int main() {
    std::cout << "Hello, C++ Server Development!" << std::endl;
    return 0;
}`;
}

// Basic syntax highlighting
function highlightSyntax(textarea) {
  // This is a simplified syntax highlighter
  // In a real implementation, you'd use a library like Prism.js
  const code = textarea.value;

  // Add some basic highlighting classes
  textarea.classList.remove("cpp-keyword", "cpp-string", "cpp-comment");

  if (
    code.includes("int") ||
    code.includes("std::") ||
    code.includes("return")
  ) {
    textarea.classList.add("cpp-keyword");
  }

  if (code.includes('"') || code.includes("'")) {
    textarea.classList.add("cpp-string");
  }

  if (code.includes("//") || code.includes("/*")) {
    textarea.classList.add("cpp-comment");
  }
}

// Initialize progress tracking
function initializeProgressTracking() {
  // Load progress from localStorage
  const progress = loadProgress();

  // Update progress bars
  updateProgressBars(progress);

  // Add lesson completion tracking
  document.querySelectorAll(".lesson-list a").forEach((link) => {
    link.addEventListener("click", () => {
      markLessonComplete(link.textContent.trim().toLowerCase());
    });
  });
}

// Load progress from localStorage
function loadProgress() {
  const saved = localStorage.getItem("cpp-course-progress");
  return saved ? JSON.parse(saved) : {};
}

// Save progress to localStorage
function saveProgress(progress) {
  localStorage.setItem("cpp-course-progress", JSON.stringify(progress));
}

// Mark lesson as complete
function markLessonComplete(lessonName) {
  const progress = loadProgress();
  progress[lessonName] = true;
  saveProgress(progress);

  // Update UI
  updateProgressBars(progress);

  // Show completion message
  showCompletionMessage(lessonName);
}

// Update progress bars
function updateProgressBars(progress) {
  const progressTrackers = document.querySelectorAll(".progress-tracker");

  progressTrackers.forEach((tracker) => {
    const progressBar = tracker.querySelector(".progress-fill");
    const progressText = tracker.querySelector("p");

    if (progressBar && progressText) {
      const totalLessons = Object.keys(progress).length;
      const completedLessons = Object.values(progress).filter(Boolean).length;
      const percentage =
        totalLessons > 0 ? (completedLessons * 100) / totalLessons : 0;

      progressBar.style.width = percentage + "%";
      progressText.textContent = `${completedLessons} of ${totalLessons} lessons completed (${Math.round(
        percentage
      )}%)`;
    }
  });
}

// Show completion message
function showCompletionMessage(lessonName) {
  const message = document.createElement("div");
  message.className = "completion-message";
  message.innerHTML = `
        <div class="completion-content">
            <h3>🎉 Lesson Completed!</h3>
            <p>Great job completing "${lessonName}"!</p>
            <button onclick="this.parentElement.parentElement.remove()" class="btn btn-primary">Continue</button>
        </div>
    `;

  // Style the message
  message.style.cssText = `
        position: fixed;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        background: white;
        padding: 2rem;
        border-radius: 12px;
        box-shadow: 0 10px 40px rgba(0,0,0,0.3);
        z-index: 10000;
        text-align: center;
    `;

  document.body.appendChild(message);

  // Auto-remove after 5 seconds
  setTimeout(() => {
    if (message.parentElement) {
      message.remove();
    }
  }, 5000);
}

// Initialize navigation features
function initializeNavigation() {
  // Add active state to current page
  const currentPath = window.location.pathname;
  const navLinks = document.querySelectorAll(".nav-menu a");

  navLinks.forEach((link) => {
    if (link.getAttribute("href") === currentPath) {
      link.style.backgroundColor = "rgba(255,255,255,0.2)";
    }
  });

  // Add mobile menu toggle
  const mobileMenuToggle = document.createElement("button");
  mobileMenuToggle.className = "mobile-menu-toggle";
  mobileMenuToggle.innerHTML = "☰";
  mobileMenuToggle.style.cssText = `
        display: none;
        background: none;
        border: none;
        color: white;
        font-size: 1.5rem;
        cursor: pointer;
    `;

  const navContainer = document.querySelector(".nav-container");
  if (navContainer) {
    navContainer.insertBefore(mobileMenuToggle, navContainer.firstChild);

    mobileMenuToggle.addEventListener("click", () => {
      const navMenu = document.querySelector(".nav-menu");
      navMenu.classList.toggle("mobile-open");
    });
  }

  // Add responsive styles
  addResponsiveStyles();
}

// Add responsive styles
function addResponsiveStyles() {
  const style = document.createElement("style");
  style.textContent = `
        @media (max-width: 768px) {
            .mobile-menu-toggle {
                display: block !important;
            }
            
            .nav-menu {
                display: none;
                position: absolute;
                top: 100%;
                left: 0;
                right: 0;
                background: #667eea;
                flex-direction: column;
                padding: 1rem;
            }
            
            .nav-menu.mobile-open {
                display: flex;
            }
            
            .nav-container {
                position: relative;
            }
        }
    `;

  document.head.appendChild(style);
}

// Utility function to show notifications
function showNotification(message, type = "info") {
  const notification = document.createElement("div");
  notification.className = `notification notification-${type}`;
  notification.textContent = message;

  notification.style.cssText = `
        position: fixed;
        top: 20px;
        right: 20px;
        background: ${
          type === "success"
            ? "#68d391"
            : type === "error"
            ? "#fc8181"
            : "#63b3ed"
        };
        color: white;
        padding: 1rem 1.5rem;
        border-radius: 8px;
        box-shadow: 0 4px 20px rgba(0,0,0,0.2);
        z-index: 10000;
        transform: translateX(100%);
        transition: transform 0.3s ease;
    `;

  document.body.appendChild(notification);

  // Animate in
  setTimeout(() => {
    notification.style.transform = "translateX(0)";
  }, 100);

  // Auto-remove after 3 seconds
  setTimeout(() => {
    notification.style.transform = "translateX(100%)";
    setTimeout(() => {
      if (notification.parentElement) {
        notification.remove();
      }
    }, 300);
  }, 3000);
}

// Add keyboard shortcuts
document.addEventListener("keydown", function (e) {
  // Ctrl/Cmd + Enter to run code
  if ((e.ctrlKey || e.metaKey) && e.key === "Enter") {
    const activeEditor = document.querySelector(".code-editor:focus-within");
    if (activeEditor) {
      const runButton = activeEditor.querySelector(".btn-primary");
      if (runButton) {
        runButton.click();
      }
    }
  }

  // Ctrl/Cmd + S to save progress
  if ((e.ctrlKey || e.metaKey) && e.key === "s") {
    e.preventDefault();
    showNotification("Progress saved automatically!", "success");
  }
});

// Add some fun Easter eggs
let konamiCode = [];
const konamiSequence = [
  "ArrowUp",
  "ArrowUp",
  "ArrowDown",
  "ArrowDown",
  "ArrowLeft",
  "ArrowRight",
  "ArrowLeft",
  "ArrowRight",
  "KeyB",
  "KeyA",
];

document.addEventListener("keydown", function (e) {
  konamiCode.push(e.code);

  if (konamiCode.length > konamiSequence.length) {
    konamiCode.shift();
  }

  if (konamiCode.join(",") === konamiSequence.join(",")) {
    showNotification(
      "🎮 Konami Code activated! You're a C++ master!",
      "success"
    );
    document.body.style.animation = "rainbow 2s infinite";

    // Add rainbow animation
    const rainbowStyle = document.createElement("style");
    rainbowStyle.textContent = `
            @keyframes rainbow {
                0% { filter: hue-rotate(0deg); }
                100% { filter: hue-rotate(360deg); }
            }
        `;
    document.head.appendChild(rainbowStyle);

    // Reset after 5 seconds
    setTimeout(() => {
      document.body.style.animation = "";
    }, 5000);

    konamiCode = [];
  }
});

console.log("Interactive features initialized successfully! 🚀");
