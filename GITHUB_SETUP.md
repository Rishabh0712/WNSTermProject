# GitHub Repository Setup Instructions

## Repository Information
- **Author:** Rishabh Kumar (cs25resch04002)
- **Email:** kumarrishabh73@gmail.com
- **Project:** 5G Simulation with Secure Syslog Integration
- **Local Git:** Already initialized and committed

---

## Steps to Create GitHub Repository

### Option 1: Using GitHub Web Interface (Recommended)

1. **Go to GitHub**
   - Navigate to: https://github.com/
   - Sign in with your account

2. **Create New Repository**
   - Click the "+" icon in the top right
   - Select "New repository"

3. **Repository Settings**
   - **Repository name:** `5g-secure-syslog-integration`
   - **Description:** "5G network simulation with secure syslog logging for real-time monitoring and analysis - Midterm Project"
   - **Visibility:** Choose Public or Private
   - **Do NOT initialize with:**
     - ✗ README (we already have one)
     - ✗ .gitignore (we already have one)
     - ✗ License

4. **Create Repository**
   - Click "Create repository"

5. **Connect Local Repository**
   After creating the repository, run these commands in PowerShell:

   ```powershell
   # Add remote repository (replace YOUR_USERNAME with your GitHub username)
   wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git remote add origin https://github.com/YOUR_USERNAME/5g-secure-syslog-integration.git"
   
   # Push to GitHub
   wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git push -u origin main"
   ```

---

### Option 2: Using GitHub CLI

If you have GitHub CLI installed:

```powershell
# Login to GitHub
wsl gh auth login

# Create repository
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && gh repo create 5g-secure-syslog-integration --public --source=. --remote=origin --push"
```

---

## Current Git Status

✅ **Git Configured:**
- Username: Rishabh Kumar (cs25resch04002)
- Email: kumarrishabh73@gmail.com

✅ **Repository Initialized:**
- Branch: main
- Initial commit: completed
- Files committed: 7

✅ **Files in Repository:**
1. `.gitignore` - Excludes large files and OAI repo
2. `README.md` - Comprehensive project documentation
3. `5G_SETUP_SUMMARY.md` - Setup and results summary
4. `midterm_presentation.pptx` - 20-slide presentation
5. `create_midterm_ppt.py` - Python script for PPT generation
6. `proposal.pdf` - Initial project proposal
7. `midterm.pdf` - Midterm deliverable template

✅ **Files Excluded:**
- `openairinterface5g/` - Too large (441 MB), excluded in .gitignore
- `.venv/` - Python virtual environment
- `*.pcap` - Packet capture files

---

## Verification Commands

Check repository status:
```powershell
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git status"
```

View commit history:
```powershell
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git log"
```

Check remote:
```powershell
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git remote -v"
```

---

## After Pushing to GitHub

### Add Additional Documentation (Optional)

Create GitHub-specific files if needed:

1. **CONTRIBUTING.md** - Contribution guidelines
2. **LICENSE** - Project license
3. **CHANGELOG.md** - Version history
4. **docs/** folder - Additional documentation

### Enable GitHub Features

- **GitHub Pages** - For documentation
- **GitHub Actions** - For CI/CD
- **Issues** - For task tracking
- **Projects** - For project management
- **Discussions** - For community interaction

---

## Repository Structure (After Push)

```
https://github.com/YOUR_USERNAME/5g-secure-syslog-integration
├── .gitignore
├── README.md
├── 5G_SETUP_SUMMARY.md
├── create_midterm_ppt.py
├── midterm_presentation.pptx
├── proposal.pdf
└── midterm.pdf
```

---

## Useful Git Commands

### Making Changes

```powershell
# Check status
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git status"

# Add new/modified files
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git add <filename>"

# Commit changes
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git commit -m 'Commit message'"

# Push to GitHub
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git push"
```

### Viewing History

```powershell
# View commit log
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git log --oneline"

# View changes
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git diff"
```

### Branching

```powershell
# Create new branch
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git checkout -b feature-branch"

# Switch branches
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git checkout main"

# Merge branch
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git merge feature-branch"
```

---

## Authentication

### Using Personal Access Token (Recommended)

1. Go to GitHub Settings → Developer settings → Personal access tokens
2. Generate new token with `repo` permissions
3. Use token as password when pushing

### Using SSH (Alternative)

```powershell
# Generate SSH key
wsl ssh-keygen -t ed25519 -C "kumarrishabh73@gmail.com"

# Copy public key
wsl cat ~/.ssh/id_ed25519.pub

# Add to GitHub: Settings → SSH and GPG keys → New SSH key
```

Then update remote URL:
```powershell
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git remote set-url origin git@github.com:YOUR_USERNAME/5g-secure-syslog-integration.git"
```

---

## Troubleshooting

### Permission Denied

If you get permission errors, use Personal Access Token or set up SSH keys.

### Large Files Warning

The OAI repository is excluded in `.gitignore`. If you need to include it, consider using Git LFS:

```powershell
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git lfs install"
```

### Conflict Resolution

If conflicts occur:
```powershell
# Pull latest changes
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git pull"

# Resolve conflicts in files
# Then commit
wsl bash -c "cd /mnt/c/Users/kumarrishabh/Documents/WNS && git add . && git commit -m 'Resolved conflicts'"
```

---

## Next Steps

1. ✅ Create GitHub repository (see Option 1 above)
2. ✅ Add remote origin
3. ✅ Push initial commit
4. ⬜ Add repository description and topics
5. ⬜ Create GitHub Issues for tasks
6. ⬜ Set up GitHub Actions (optional)
7. ⬜ Add collaborators (if needed)

---

## Support

For Git/GitHub help:
- Git Documentation: https://git-scm.com/doc
- GitHub Docs: https://docs.github.com/
- GitHub Skills: https://skills.github.com/

---

**Ready to push!** Follow Option 1 above to create your GitHub repository.
