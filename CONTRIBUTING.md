## How to add changes to the main repository

### Creating a fork
1. Create a fork of the main repository.
2. Clone your fork to your local computer.

### Updating your fork
In your fork, add a remote called `upstream`, which should be a link to the main repository. This remote will keep your fork up-to-date with the latest changes that are added to the main repository.

It is good to update your fork as much as possible so your changes are compatible with the main repository; otherwise, the main repository will become more different than your fork and it becomes harder to resolve the differences between the two before you can apply your changes.

`git remote add upstream https://github.com/FormulaRacingAtUCDavis/VCU-Firmware-FE9`

To update your fork, download (fetch) the latest changes of the main repository to a local copy on your computer. Then, "rebase" your desired changes (commits) on top of the latest changes in the main repository. Here are the steps:

1. `git fetch upstream`
2. `git rebase upstream/main`

### Making changes
Create a **new** branch just for the changes you want to make. By doing this, you still have access to the main branch without those changes.

### Publish changes
In your fork's homepage on GitHub, create a pull request to the main repository.
