# Multiplication-Facts-SRS
This is a Windows Multiplication Facts Spaced Repetition System

## Project overview
The Multiplication Facts Trainer is a native Win32 (C++17) desktop app that drills all multiplication facts from 0 to 12. It shows a prompt, lets you reveal the answer, and records a Good/Meh/Bad rating for each card.

### Features
- Generates every 0–12 multiplication pairing and shuffles the deck on startup.
- Question and answer text boxes sized for readability; the answer stays hidden until requested.
- "Show Answer" button plus keyboard shortcuts (Space/Enter) to reveal answers.
- Rating buttons (Good/Meh/Bad) and keyboard shortcuts (1/2/3) that log a timestamped event to `ratings.log` and advance to the next card.
- Esc exits immediately; hotkeys work regardless of focus.
- Resizable window with a minimum client size of 640×480; layout adjusts on resize.

### Building
1. Open `MultiplicationFactsTrainer.sln` in Visual Studio 2022 (or later) on Windows 10+.
2. Build either the Debug or Release x64 configuration. The project uses the static runtime (`/MT` variants) and raw Win32 APIs only.
3. Run the produced `MultiplicationFactsTrainer.exe` from the IDE or the output folder.

### CI and releases
- GitHub Actions builds the Release x64 executable on `windows-latest` for every tag that starts with `v` (for example, `v1.0.0`).
- Tagging a commit automatically publishes a GitHub Release with the zipped `MultiplicationFactsTrainer.exe` attached as a download.
- You can also run the workflow manually from the Actions tab (workflow_dispatch) to produce a downloadable artifact from any branch.

### Usage notes
- The answer box is read-only and disabled until you click **Show Answer** (or press Space/Enter).
- After rating a card, the app automatically loads the next one; when you reach the end of the shuffled deck it restarts from the beginning.
- Ratings are appended to `ratings.log` in the working directory using the format `timestamp|cardIndex|rating`.
