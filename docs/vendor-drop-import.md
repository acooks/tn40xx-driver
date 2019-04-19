# How to add a vendor drop:
- Check for new tarball on http://www.tehutinetworks.net/?t=drivers&L1=8&L2=12&L3=26
- Download and extract tarball
- Create a new empty branch with `git checkout --orphan vendor-drop/v0.3.XYZ`
- Unstage any changes with `git reset --hard`
- CAREFULLY clean out all left-over files: `git clean -fd`
- Copy the extracted sources to the git repo.
- Add all of it: `git add .`
- Commit

# How to run cleanup:
- Start from the vendor-drop: `git checkout vendor-drop/v0.3.XYZ`
- Create and Checkout the cleanup branch: `git checkout -b cleanup/v0.3.XYZ`
- Merge master: `git merge --allow-unrelated-histories master`
- Run cleanup: `LINDENT=~/linux/scripts/Lindent ./cleanup/cleanup.sh`
