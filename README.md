# Private Watering System

## Setup Instructions

### 1. Configure Git Settings

Before using this repository, you need to update the `.gitconfig` file with your personal information:

1. Open `.gitconfig` in your editor
2. Replace the following placeholders with your actual information:
   - `Your Name` → Your actual name
   - `your.email@example.com` → Your GitHub email address
   - `your-github-username` → Your GitHub username

### 2. Set Up GitHub Authentication

#### Option A: Using GitHub CLI (Recommended)
1. Install GitHub CLI: https://cli.github.com/
2. Run: `gh auth login`
3. Follow the prompts to authenticate with your GitHub account

#### Option B: Using Personal Access Token
1. Go to GitHub Settings → Developer settings → Personal access tokens
2. Generate a new token with appropriate permissions (repo, workflow, etc.)
3. When prompted for password, use this token instead

#### Option C: Using SSH Keys
1. Generate SSH key: `ssh-keygen -t ed25519 -C "your.email@example.com"`
2. Add the public key to your GitHub account
3. Update remote URL: `git remote set-url origin git@github.com:username/repo.git`

### 3. Verify Setup

Run the following commands to verify your setup:

```bash
git config --list
gh auth status  # if using GitHub CLI
```

### 4. Repository Configuration

The `.gitconfig` file includes:
- Credential helper for secure authentication
- Default branch set to `main`
- Auto-setup remote for new branches
- VS Code as default editor and merge tool
- Proper line ending handling for Windows

## Project Structure

```
private_watering_system/
├── .git/
├── .gitconfig          # Git configuration for this repository
├── .gitignore          # Files to ignore in version control
└── README.md           # This file
```

## Notes

- The `.gitconfig` file is repository-specific and won't affect your global Git settings
- Credentials are stored securely using the credential manager
- The `.gitignore` file excludes common development artifacts and sensitive files 