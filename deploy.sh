#!/bin/bash

# Git Push script for Salim Omer Payment System
# This script adds, commits, and pushes changes to remote repository

set -e  # Exit on any error

echo "🚀 Starting git push process..."

# Step 1: Check if we're in a git repository
if [ ! -d ".git" ]; then
    echo "❌ Error: Not a git repository. Please run 'git init' first."
    exit 1
fi

# Step 2: Check for uncommitted changes
echo "📝 Checking for changes..."
if [ -z "$(git status --porcelain)" ]; then
    echo "ℹ️  No changes to commit."
    echo "🔍 Checking if we need to push existing commits..."
    
    # Check if there are unpushed commits
    if [ -z "$(git log origin/main..HEAD 2>/dev/null)" ]; then
        echo "✅ Everything is up to date!"
        exit 0
    fi
else
    # Step 3: Add all files
    echo "📁 Adding all files to git..."
    git add .
    
    # Step 4: Create commit with timestamp
    echo "💾 Creating commit..."
    commit_message="Update: $(date '+%Y-%m-%d %H:%M:%S')"
    git commit -m "$commit_message"
fi

# Step 5: Check if remote origin exists
echo "🔗 Checking remote origin..."
if ! git remote get-url origin > /dev/null 2>&1; then
    echo "🔗 Adding remote origin..."
    git remote add origin https://github.com/jansencodez/sasaride.git
fi

# Step 6: Push to remote
echo "⬆️ Pushing to remote repository..."
git branch -M main
git push -u origin main

echo "✅ Git push completed successfully!"
echo "🎉 Your changes have been pushed to GitHub"
