# Codespace Coordination Workflow

## Problem
Multiple local codespace instances can create diverging changes, merge conflicts, and lost work.

## Solution: Coordinated Feature Branch Workflow

### 🔄 **Daily Workflow**

#### Before Starting Work (Every Session)
```bash
# 1. Always start by syncing with remote
git fetch --all
git status

# 2. Check what branch you're on
git branch --show-current

# 3. If on shared branch, pull latest changes
git pull origin [current-branch]

# 4. Create a new feature branch for your work session
git checkout -b feature/[session-description]-$(date +%m%d-%H%M)
# Example: feature/economic-system-fixes-1013-2045
```

#### During Work Session
```bash
# Commit frequently with descriptive messages
git add .
git commit -m "Specific description of what was changed"

# Push your feature branch regularly
git push origin feature/[your-branch-name]
```

#### End of Session (Before Closing Codespace)
```bash
# 1. Commit all changes
git add .
git commit -m "End of session: [summary of work done]"

# 2. Push your feature branch
git push origin feature/[your-branch-name]

# 3. Create summary of your work
echo "## Work Summary $(date)" >> WORK-SESSION-LOG.md
echo "Branch: $(git branch --show-current)" >> WORK-SESSION-LOG.md
echo "Changes: [brief description]" >> WORK-SESSION-LOG.md
echo "Status: [completed/in-progress/blocked]" >> WORK-SESSION-LOG.md
echo "Next: [what should be done next]" >> WORK-SESSION-LOG.md
echo "" >> WORK-SESSION-LOG.md
```

### 🤝 **Coordination Between Codespaces**

#### Option 1: Sequential Work (Recommended)
- Only one person works at a time
- Complete your session and push before other starts
- Other person pulls latest changes before starting

#### Option 2: Parallel Work (Advanced)
- Work on different systems/files
- Use specific feature branches
- Coordinate via communication which files/systems each person is working on
- Merge feature branches one at a time

### 📋 **Integration Checklist**

Before merging any feature branch:
```bash
# 1. Ensure code builds successfully
cd build && make clean && make -j$(nproc)

# 2. Run tests if available
# make test

# 3. Update documentation
# - PROJECT-STATUS.md
# - CHANGELOG.md  

# 4. Create pull request or merge to main shared branch
```

### 🚨 **Emergency Recovery**

If codespaces diverge badly:
```bash
# Save your work
git stash push -m "Emergency backup $(date)"
git push origin HEAD:emergency-backup-$(date +%m%d-%H%M)

# Reset to known good state  
git fetch origin
git reset --hard origin/main  # or origin/[shared-branch]

# Re-apply your work from backup
git checkout emergency-backup-[timestamp]
git cherry-pick [specific-commits]
```

## Current Repository State

### Active Branches
- `main` - Latest stable integration
- `copilot/review-architectural-files` - Current shared work branch
- `feature/threading-system-integration` - Threading work (can be cleaned up)

### Integrated Systems Status
- ✅ ECS Core (EntityManager, ComponentAccessManager, MessageBus)
- ✅ Administrative System 
- ✅ Military System
- ✅ Population System
- ✅ Threading System (implemented but disabled in CMake)
- ⚠️ Economic System (needs interface fixes)

### Next Priority Work
1. Fix Economic System interface mismatches
2. Enable Threading System in CMakeLists.txt
3. Add Technology System integration
4. Resolve JSON dependency issues

## Communication Protocol

### Before Each Session
1. Check WORK-SESSION-LOG.md for latest status
2. Check what branch has the latest work
3. Communicate which systems you plan to work on

### During Session
- Update WORK-SESSION-LOG.md with progress
- Push commits regularly
- Note any blocking issues

### End of Session  
- Update WORK-SESSION-LOG.md with completion status
- Push all changes
- Document next steps for continuation